from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
try:
    from .Accounting import AccountingControl
except ImportError:
    AccountingControl = None
try:
    from .DataStaging import DataStagingControl
except ImportError:
    DataStagingControl = None


import subprocess
import sys
import re
import pickle
import time
import pwd
import signal

try:
   input = raw_input  # Redefine for Python 2
except NameError:
   pass


def complete_job_owner(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return JobsControl(arcconf).complete_owner(parsed_args)


def complete_job_id(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return JobsControl(arcconf).complete_job(parsed_args)


class JobsControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Jobs')
        self.control_dir = None
        self.arcconfig = arcconfig
        # arcctl is inside arex package as well as gm-jobs
        self.gm_jobs = ARC_LIBEXEC_DIR + '/gm-jobs'
        if not os.path.exists(self.gm_jobs):
            self.logger.error('A-REX gm-jobs is not found at %s. It seams you A-REX install is broken.', self.gm_jobs)
            sys.exit(1)
        # config is mandatory
        if arcconfig is None:
            self.logger.error('Failed to get parsed arc.conf. Jobs control is not possible.')
            sys.exit(1)
        # controldir is mandatory
        self.control_dir = self.arcconfig.get_value('controldir', 'arex').rstrip('/')
        if self.control_dir is None:
            self.logger.critical('Jobs control is not possible without controldir.')
            sys.exit(1)
        self.logger.debug('Using controldir location: %s', self.control_dir)
        # construct the path to A-REX runtime configuration
        # using configuration other that A-REX has is not consistent
        arex_pidfile = self.arcconfig.get_value('pidfile', 'arex')
        controldir_fallback = True
        if arex_pidfile is not None:
            arex_runconf = arex_pidfile.rsplit('.', 1)[0] + '.cfg'
            if os.path.exists(arex_runconf):
                self.logger.debug('Using A-REX runtime configuration (%s) for managing jobs.', arex_runconf)
                controldir_fallback = False
                self.gm_jobs += ' -c {0}'.format(arex_runconf)
        if controldir_fallback:
            self.logger.warning('A-REX runtime configuration is not found. Falling back to directly using '
                                'configured controldir at %s', self.control_dir)
            self.gm_jobs += ' -d {0}'.format(self.control_dir)

        self.cache_min_jobs = 1000
        self.cache_ttl = 30
        self.jobs = {}
        self.process_job_log_file = False   # dummy assignment for job_log follow

    def __get_config_value(self, block, option, default_value=None):
        value = self.arcconfig.get_value(option, block)
        if value is None:
            value = default_value
        return value

    def __run_gmjobs(self, args, stderr=False):
        __GMJOBS = self.gm_jobs.split()
        loglevel = logging.getLogger('ARCCTL').getEffectiveLevel()
        __GMJOBS += ['-x', {50: 'FATAL', 40: 'ERROR', 30: 'WARNING', 20: 'INFO', 10: 'DEBUG'}[loglevel]]
        __GMJOBS += args.split()
        self.logger.debug('Running %s', ' '.join(__GMJOBS))
        if stderr:
            return subprocess.Popen(__GMJOBS, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return subprocess.Popen(__GMJOBS, stdout=subprocess.PIPE)

    @staticmethod
    def __xargs_jobs(joblist, cmdarg):
        # leave enough headroom to not bump into MAX_ARG_STRLEN (131072)
        _MAX_ARG_STRLEN_LIMIT = 100000
        # loop over argument list and create argument string with proper length
        argstring = ''
        for arg in joblist:
            argstring += ' ' + cmdarg + ' ' + arg['id']
            if len(argstring) > _MAX_ARG_STRLEN_LIMIT:
                yield argstring
                argstring = ''
        yield argstring

    @staticmethod
    def __list_job(job_dict, list_long=False):
        if list_long:
            print('{id} {state:>13}\t{name:<32}\t{userdn}'.format(**job_dict))
        else:
            print(job_dict['id'])

    def __job_exists(self, jobid):
        if jobid not in self.jobs:
            self.logger.error('There is no such job %s', jobid)
            sys.exit(1)

    def __parse_job_attrs(self, jobid, attrtype='local'):
        _KEYVALUE_RE = re.compile(r'^([^=]+)=(.*)$')
        attr_file = '{0}/job.{1}.{2}'.format(self.control_dir, jobid, attrtype)
        job_attrs = {}
        if os.path.exists(attr_file):
            with open(attr_file, 'r') as attr_f:
                for line in attr_f:
                    kv = _KEYVALUE_RE.match(line)
                    if kv:
                        job_attrs[kv.group(1)] = kv.group(2).strip("'\"")
            if attrtype == 'local':
                job_attrs['mapped_account'] = pwd.getpwuid(os.stat(attr_file).st_uid).pw_name
        else:
            self.__get_jobs()
            self.__job_exists(jobid)
            self.logger.error('Failed to open job attributes file: %s', attr_file)
        return job_attrs

    def _service_log_print(self, log_path, jobid):
        print('### ' + log_path + ':')
        if os.path.exists(log_path):
            with open(log_path, 'r') as log_f:
                for line in log_f:
                    if jobid in line:
                        sys.stdout.write(line)
            sys.stdout.flush()
        else:
            self.logger.error('Failed to open service log file: %s', log_path)

    def __get_jobs(self):
        # check cache first
        __cache_file = '/tmp/.arcctl.jobs.cache'
        if os.path.exists(__cache_file):
            time_allowed = time.time() - self.cache_ttl
            if os.stat(__cache_file).st_mtime > time_allowed:
                with open(__cache_file, 'rb') as cfd:
                    self.logger.debug('Using cached jobs information (cache valid for %s seconds)', self.cache_ttl)
                    self.jobs = pickle.load(cfd)
                    return
        # invoke gm-jobs and parse the list of jobs
        self.jobs = {}
        __JOB_RE = re.compile(r'^Job:\s*(.*)\s*$')
        __JOB_ATTRS = {
            'state': re.compile(r'^\s*State:\s*(.*)\s*$'),
            'modified': re.compile(r'^\s*Modified:\s*(.*)\s*$'),
            'userdn': re.compile(r'^\s*User:\s*(.*)\s*$'),
            'lrmsid': re.compile(r'^\s*LRMS id:\s*(.*)\s*$'),
            'name': re.compile(r'^\s*Name:\s*(.*)\s*$'),
            'from': re.compile(r'^\s*From:\s*(.*)\s*$'),
        }
        gmjobs_out = self.__run_gmjobs('--longlist --notshowstates')
        job_dict = {}
        for rline in iter(gmjobs_out.stdout.readline, b''):
            line = rline.decode('utf-8')
            jobre = __JOB_RE.match(line)
            if jobre:
                if job_dict:
                    for attr in __JOB_ATTRS:
                        if attr not in job_dict:
                            job_dict[attr] = 'N/A'
                    self.jobs[job_dict['id']] = job_dict
                job_dict = {'id': jobre.group(1)}
                continue
            for attr, regex in __JOB_ATTRS.items():
                attr_re = regex.match(line)
                if attr_re:
                    job_dict[attr] = attr_re.group(1)
        if job_dict:
            for attr in __JOB_ATTRS:
                if attr not in job_dict:
                    job_dict[attr] = 'N/A'
            self.jobs[job_dict['id']] = job_dict
        # dump jobs dictionary to cache in case there are many jobs
        if len(self.jobs) > self.cache_min_jobs:
            with open(__cache_file, 'wb') as cfd:
                self.logger.debug('Dumping jobs information to cache')
                pickle.dump(self.jobs, cfd)

    def __filtered_jobs(self, args):
        for job_d in self.jobs.values():
            if hasattr(args, 'state'):
                if args.state:
                    if job_d['state'] not in args.state:
                        continue
            if hasattr(args, 'owner'):
                if args.owner:
                    if job_d['userdn'] not in args.owner:
                        continue
            yield job_d

    def list(self, args):
        self.__get_jobs()
        for job_d in self.__filtered_jobs(args):
            self.__list_job(job_d, args.long)

    def kill_or_clean(self, args, action='-k'):
        self.__get_jobs()
        for jobid in args.jobid:
            self.__job_exists(jobid)
        __JOB_RE = re.compile(r'^Job:\s*')
        gmjobs_out = self.__run_gmjobs('-J -S ' + action + ' ' + ' '.join(args.jobid))
        for line in iter(gmjobs_out.stdout.readline, b''):
            if __JOB_RE.match(line.decode('utf-8')):
                sys.stdout.write(line)
        sys.stdout.flush()

    def kill_or_clean_all(self, args, action='-k'):
        # safe check when killing/cleaning all jobs
        if not args.owner and not args.state:
            reply = str(input('You have not specified any filters and operation will continue for ALL A-REX jobs. '
                              'Please type "yes" if it is desired behaviour: '))
            if reply != 'yes':
                sys.exit(0)
        self.__get_jobs()
        __JOB_RE = re.compile(r'^Job:\s*')
        for argstr in self.__xargs_jobs(self.__filtered_jobs(args), action):
            gmjobs_out = self.__run_gmjobs('-J -S' + argstr)
            for line in iter(gmjobs_out.stdout.readline, b''):
                if __JOB_RE.match(line.decode('utf-8')):
                    sys.stdout.write(line)
            sys.stdout.flush()

    def job_script(self, args):
        error_log = '{0}/job.{1}.errors'.format(self.control_dir, args.jobid)
        if os.path.exists(error_log):
            el_f = open(error_log, 'r')
            print_line = False
            for line in el_f:
                if line.startswith('----- starting submit'):
                    print_line = True
                    continue
                if line.startswith('----- exiting submit'):
                    break
                if print_line:
                    sys.stdout.write(line)
            sys.stdout.flush()
            if not print_line:
                self.logger.error('There is no job script found in log file. Is the job reached LRMS?')
        else:
            self.__get_jobs()
            self.__job_exists(args.jobid)
            self.logger.error('Failed to find job log file containing generated job script: %s', error_log)

    @staticmethod
    def __cut_jobscript(line, print_line):
        # modifies [print flag, continue flag] list (passed by reference)
        if line.startswith('----- starting submit'):
            print_line[0] = False
            print_line[1] = True
        if line.startswith('----- exiting submit'):
            print_line[0] = True
            print_line[1] = True
        # keep print flag but remove continue
        print_line[1] = False

    def __job_log_signal_handler(self, signum, frame):
        self.process_job_log_file = False

    def __follow_log(self, log, follow=False, process_function=None):
        self.process_job_log_file = True
        if os.path.exists(log):
            el_f = open(log, 'r')
            print_line = [True, False]  # [print flag, continue flag]
            pos = 0
            if follow:
                signal.signal(signal.SIGINT, self.__job_log_signal_handler)
            while self.process_job_log_file:
                el_f.seek(pos)
                for line in el_f:
                    if process_function is not None:
                        process_function(line, print_line)
                    if print_line[1]:
                        continue
                    if print_line[0]:
                        sys.stdout.write(line)
                sys.stdout.flush()
                pos = el_f.tell()
                if not follow:
                    self.process_job_log_file = False
                else:
                    time.sleep(0.1)
            el_f.close()
        else:
            self.logger.error('Failed to find log file: %s', log)

    def job_log(self, args):
        error_log = '{0}/job.{1}.errors'.format(self.control_dir, args.jobid)
        if os.path.exists(error_log):
            self.__follow_log(error_log, args.follow, process_function=self.__cut_jobscript)
        else:
            self.__get_jobs()
            self.__job_exists(args.jobid)
            self.logger.error('Failed to find job log file: %s', error_log)

    def job_service_logs(self, args):
        # A-REX main log
        arex_log = self.__get_config_value('arex', 'logfile', '/var/log/arc/arex.log')
        self._service_log_print(arex_log, args.jobid)
        # WS interface logs
        if self.arcconfig.check_blocks('arex/ws'):
            arexws_log = self.__get_config_value('arex/ws', 'logfile', '/var/log/arc/ws-interface.log')
            self._service_log_print(arexws_log, args.jobid)
        # GridFTP interface logs
        if self.arcconfig.check_blocks('gridftpd'):
            gridftpd_log = self.__get_config_value('gridftpd', 'logfile', '/var/log/arc/gridftpd.log')
            self._service_log_print(gridftpd_log, args.jobid)
        # A-REX jobs log
        arexjob_log = self.__get_config_value('a-rex', 'joblog')
        if arexjob_log is not None:
            self._service_log_print(arexjob_log, args.jobid)

    def jobinfo(self, args):
        self.__get_jobs()
        self.__job_exists(args.jobid)
        print('Name\t\t: {name}\n' \
              'Owner\t\t: {userdn}\n' \
              'State\t\t: {state}\n' \
              'LRMS ID\t\t: {lrmsid}\n' \
              'Modified\t: {modified}'.format(**self.jobs[args.jobid]))

    def job_stdout(self, args):
        job_grami = self.__parse_job_attrs(args.jobid, 'grami')
        if not 'joboption_stdout' in job_grami:
            self.logger.error('Cannot find executable stdout location for job %s', args.jobid)
            sys.exit(1)
        self.__follow_log(job_grami['joboption_stdout'], args.follow)

    def job_stderr(self, args):
        job_grami = self.__parse_job_attrs(args.jobid, 'grami')
        if not 'joboption_stderr' in job_grami:
            self.logger.error('Cannot find executable stderr location for job %s', args.jobid)
            sys.exit(1)
        self.__follow_log(job_grami['joboption_stderr'], args.follow)

    def job_getattr(self, args):
        job_attrs = self.__parse_job_attrs(args.jobid)
        if args.attr:
            if args.attr in job_attrs:
                print(job_attrs[args.attr])
            else:
                self.logger.error('There is no such attribute \'%s\' defined for job %s', args.attr, args.jobid)
        else:
            for k, v in job_attrs.items():
                print('{0:<32}: {1}'.format(k, v))

    def job_stats(self, args):
        __RE_JOBSTATES = re.compile(r'^\s*([A-Z]+):\s+([0-9]+)\s+\(([0-9]+)\)\s*$')
        __RE_TOTALSTATS = re.compile(r'^\s*([A-Za-z]+):\s+([0-9]+)/([-0-9]+)\s*$')
        __RE_DATASTATS = re.compile(r'^\s*Processing:\s+([0-9]+)\+([0-9]+)\s*$')
        jobstates = []
        totalstats = []
        data_download = 0
        data_upload = 0
        # collect information from gm-jobs
        gmjobs_out = self.__run_gmjobs('-J')
        for rline in iter(gmjobs_out.stdout.readline, b''):
            line = rline.decode('utf-8')
            js_re = __RE_JOBSTATES.match(line)
            if js_re:
                state = js_re.group(1)
                jobstates.append({'processing': js_re.group(2), 'waiting': js_re.group(3), 'state': state})
                continue
            t_re = __RE_TOTALSTATS.match(line)
            if t_re:
                limit = t_re.group(3)
                if limit == '-1':
                    limit = 'unlimited'
                totalstats.append({'jobs': t_re.group(2), 'limit': limit, 'state': t_re.group(1)})
                continue
            ds_re = __RE_DATASTATS.match(line)
            if ds_re:
                data_download = ds_re.group(1)
                data_upload = ds_re.group(2)
        # show total stats if requested
        if args.total:
            for t in totalstats:
                if args.long:
                    if t['state'] == 'Accepted':
                        t['state'] = 'Total number of jobs accepted for further processing by A-REX'
                    elif t['state'] == 'Running':
                        t['state'] = 'Total number of jobs running in LRMS backend'
                    elif t['state'] == 'Total':
                        t['state'] = 'Total number of jobs managed by A-REX (including completed)'
                    print('{state}\n  Jobs:  {jobs:>15}\n  Limit: {limit:>15}'.format(**t))
                else:
                    print('{state:>11}: {jobs:>8} of {limit}'.format(**t))
        # show datastaging stats
        elif args.data_staging:
            if args.long:
                print('Processing jobs in data-staging:')
                print('  Downloading: {0:>9}'.format(data_download))
                print('  Uploading:   {0:>9}'.format(data_upload))
                # add detailed stats from gm-jobs on long output
                gmjobs_out = self.__run_gmjobs('-s')
                for line in iter(gmjobs_out.stdout.readline, b''):
                    print(line.decode('utf-8'), end='')
            else:
                print('{0:>11}: {1:>8}'.format('Downloading', data_download))
                print('{0:>11}: {1:>8}'.format('Uploading', data_upload))
        # show general stats per-state by default
        else:
            for s in jobstates:
                if args.long:
                    print('{state}\n  Processing: {processing:>10}\n  Waiting:    {waiting:>10}'.format(**s))
                else:
                    print('{state:>11}: {processing:>8} ({waiting})'.format(**s))

    def control(self, args):
        self.cache_ttl = args.cachettl
        if args.action == 'list':
            self.list(args)
        elif args.action == 'killall':
            self.kill_or_clean_all(args, '-k')
        elif args.action == 'cleanall':
            self.kill_or_clean_all(args, '-r')
        elif args.action == 'kill':
            self.kill_or_clean(args, '-k')
        elif args.action == 'clean':
            self.kill_or_clean(args, '-r')
        elif args.action == 'info':
            self.jobinfo(args)
        elif args.action == 'script':
            self.job_script(args)
        elif args.action == 'log':
            if args.service:
                self.job_service_logs(args)
            else:
                self.job_log(args)
        elif args.action == 'stdout':
            self.job_stdout(args)
        elif args.action == 'stderr':
            self.job_stderr(args)
        elif args.action == 'attr':
            self.job_getattr(args)
        elif args.action == 'stats':
            self.job_stats(args)
        elif args.action == 'accounting' and AccountingControl is not None:
            AccountingControl(self.arcconfig).jobcontrol(args)
        elif args.action == 'datastaging' and DataStagingControl is not None:
            DataStagingControl(self.arcconfig).jobcontrol(args)

    def complete_owner(self, args):
        owners = []
        self.__get_jobs()
        for job_d in self.__filtered_jobs(args):
            if job_d['userdn'] not in owners:
                owners.append(job_d['userdn'])
        return owners

    def complete_job(self, args):
        self.__get_jobs()
        joblist = []
        for job_d in self.__filtered_jobs(args):
            joblist.append(job_d['id'])
        return joblist

    @staticmethod
    def register_parser(root_parser):
        __JOB_STATES = ['ACCEPTED', 'PREPARING', 'SUBMIT', 'INLRMS', 'FINISHING', 'FINISHED', 'DELETED', 'CANCELING']

        jobs_ctl = root_parser.add_parser('job', help='A-REX Jobs')
        jobs_ctl.add_argument('-t', '--cachettl', action='store', type=int, default=30,
                              help='GM-Jobs output caching validity in seconds (default is %(default)s)')
        jobs_ctl.set_defaults(handler_class=JobsControl)

        jobs_actions = jobs_ctl.add_subparsers(title='Jobs Control Actions', dest='action',
                                               metavar='ACTION', help='DESCRIPTION')
        jobs_actions.required = True

        jobs_list = jobs_actions.add_parser('list', help='List available A-REX jobs')
        jobs_list.add_argument('-l', '--long', help='Detailed listing of jobs', action='store_true')
        jobs_list.add_argument('-s', '--state', help='Filter jobs by state', action='append', choices=__JOB_STATES)
        jobs_list.add_argument('-o', '--owner', help='Filter jobs by owner').completer = complete_job_owner

        jobs_script = jobs_actions.add_parser('script', help='Display job script submitted to LRMS')
        jobs_script.add_argument('jobid', help='Job ID').completer = complete_job_id

        jobs_log = jobs_actions.add_parser('log', help='Display job log')
        jobs_log.add_argument('jobid', help='Job ID').completer = complete_job_id
        jobs_log.add_argument('-f', '--follow', help='Follow the job log output', action='store_true')
        jobs_log.add_argument('-s', '--service', help='Show ARC CE logs containing the jobID instead of job log',
                              action='store_true')

        jobs_info = jobs_actions.add_parser('info', help='Show job main info')
        jobs_info.add_argument('jobid', help='Job ID').completer = complete_job_id

        jobs_stdout = jobs_actions.add_parser('stdout', help='Show job executable stdout')
        jobs_stdout.add_argument('jobid', help='Job ID').completer = complete_job_id
        jobs_stdout.add_argument('-f', '--follow', help='Follow the job log output', action='store_true')

        jobs_stderr = jobs_actions.add_parser('stderr', help='Show job executable stderr')
        jobs_stderr.add_argument('jobid', help='Job ID').completer = complete_job_id
        jobs_stderr.add_argument('-f', '--follow', help='Follow the job log output', action='store_true')

        jobs_attr = jobs_actions.add_parser('attr', help='Get ')
        jobs_attr.add_argument('jobid', help='Job ID').completer = complete_job_id
        jobs_attr.add_argument('attr', help='Attribute name', nargs='?')

        jobs_kill = jobs_actions.add_parser('kill', help='Cancel job')
        jobs_kill.add_argument('jobid', nargs='+', help='Job ID').completer = complete_job_id

        jobs_killall = jobs_actions.add_parser('killall', help='Cancel all jobs')
        jobs_killall.add_argument('-s', '--state', help='Filter jobs by state', action='append', choices=__JOB_STATES)
        jobs_killall.add_argument('-o', '--owner', help='Filter jobs by owner').completer = complete_job_owner

        jobs_clean = jobs_actions.add_parser('clean', help='Clean job')
        jobs_clean.add_argument('jobid', nargs='+', help='Job ID').completer = complete_job_id

        jobs_cleanall = jobs_actions.add_parser('cleanall', help='Clean all jobs')
        jobs_cleanall.add_argument('-s', '--state', help='Filter jobs by state', action='append', choices=__JOB_STATES)
        jobs_cleanall.add_argument('-o', '--owner', help='Filter jobs by owner').completer = complete_job_owner

        jobs_stats = jobs_actions.add_parser('stats', help='Show jobs statistics')
        jobs_stats.add_argument('-l', '--long', help='Detailed output of stats', action='store_true')
        jobs_stats_type = jobs_stats.add_mutually_exclusive_group()
        jobs_stats_type.add_argument('-t', '--total', help='Show server total stats', action='store_true')
        jobs_stats_type.add_argument('-d', '--data-staging', help='Show server datastaging stats', action='store_true')

        if AccountingControl is not None:
            # add 'job accounting xxx' functionality as well as 'accounting job xxx'
            jobs_accounting = jobs_actions.add_parser('accounting', help='Show job accounting data')
            AccountingControl.register_job_parser(jobs_accounting)


        if DataStagingControl is not None:
        # add 'job datastaging xxx' functionality as well ass 'datastaging job xxx' 
            dds_job_ctl = jobs_actions.add_parser('datastaging',help='Job Datastaging Information for jobs preparing or running.')
            DataStagingControl.register_job_parser(dds_job_ctl)

            
