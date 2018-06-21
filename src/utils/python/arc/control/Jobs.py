from ControlCommon import *
import subprocess
import sys
import re
import pickle
import time
import pwd


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
        if not os.path.exists(ARC_LIBEXEC_DIR + '/gm-jobs'):
            self.logger.error('A-REX gm-jobs is not found at %s. Please ensure you have A-REX installed.',
                              ARC_LIBEXEC_DIR + '/gm-jobs')
            sys.exit(1)
        if arcconfig is None:
            self.logger.warning('Failed to get parsed arc.conf. Falling back to gm-jobs provided controldir value.')
            _CONTROLDIR_RE = re.compile(r'Control dir\s+:\s+(.*)\s*$')
            gmjobs_out = self.__run_gmjobs('--notshowstates --notshowjobs -x INFO', stderr=True)
            for line in iter(gmjobs_out.stdout.readline, ''):
                controldir = _CONTROLDIR_RE.search(line)
                if controldir:
                    self.control_dir = controldir.group(1)
                    break
        else:
            self.control_dir = self.arcconfig.get_value('controldir', 'arex').rstrip('/')
        if self.control_dir is None:
            self.logger.critical('Jobs control cannot work without controldir.')
            sys.exit(1)
        self.logger.debug('Using controldir location: %s', self.control_dir)
        self.cache_ttl = 30
        self.jobs = {}

    def __get_config_value(self, block, option, default_value=None):
        value = self.arcconfig.get_value(option, block)
        if value is None:
            value = default_value
        return value

    @staticmethod
    def __run_gmjobs(args, stderr=False):
        __GMJOBS = [ARC_LIBEXEC_DIR + '/gm-jobs']
        loglevel = logging.getLogger('ARCCTL').getEffectiveLevel()
        __GMJOBS += ['-x', {50: 'FATAL', 40: 'ERROR', 30: 'WARNING', 20: 'INFO', 10: 'DEBUG'}[loglevel]]
        if stderr:
            return subprocess.Popen(__GMJOBS + args.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return subprocess.Popen(__GMJOBS + args.split(), stdout=subprocess.PIPE)

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
            print '{id} {state:>13}\t{name:<32}\t{userdn}'.format(**job_dict)
        else:
            print job_dict['id']

    def __job_exists(self, jobid):
        if jobid not in self.jobs:
            self.logger.error('There is no such job %s', jobid)
            sys.exit(1)

    def __parse_job_attrs(self, jobid):
        _KEYVALUE_RE = re.compile(r'^([^=]+)=(.*)$')
        local_file = '{}/job.{}.local'.format(self.control_dir, jobid)
        job_attrs = {}
        if os.path.exists(local_file):
            with open(local_file, 'r') as local_f:
                for line in local_f:
                    kv = _KEYVALUE_RE.match(line)
                    if kv:
                        job_attrs[kv.group(1)] = kv.group(2)
            job_attrs['mapped_account'] = pwd.getpwuid(os.stat(local_file).st_uid).pw_name
        else:
            self.__get_jobs()
            self.__job_exists(jobid)
            self.logger.error('Failed to open job attributes file: %s', local_file)
        return job_attrs

    def _service_log_print(self, log_path, jobid):
        print '### ' + log_path + ':'
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
        for line in iter(gmjobs_out.stdout.readline, ''):
            jobre = __JOB_RE.match(line)
            if jobre:
                if job_dict:
                    for attr in __JOB_ATTRS.keys():
                        if attr not in job_dict:
                            job_dict[attr] = 'N/A'
                    self.jobs[job_dict['id']] = job_dict
                job_dict = {'id': jobre.group(1)}
                continue
            for attr, regex in __JOB_ATTRS.iteritems():
                attr_re = regex.match(line)
                if attr_re:
                    job_dict[attr] = attr_re.group(1)
        if job_dict:
            for attr in __JOB_ATTRS.keys():
                if attr not in job_dict:
                    job_dict[attr] = 'N/A'
            self.jobs[job_dict['id']] = job_dict
        # dump jobs dictionary to cache
        with open(__cache_file, 'wb') as cfd:
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
        for line in iter(gmjobs_out.stdout.readline, ''):
            if __JOB_RE.match(line):
                sys.stdout.write(line)
        sys.stdout.flush()

    def kill_or_clean_all(self, args, action='-k'):
        # safe check when killing/cleaning all jobs
        if not args.owner and not args.state:
            reply = str(raw_input('You have not specified any filters and operation will continue for ALL A-REX jobs. '
                                  'Please type "yes" if it is desired behaviour: '))
            if reply != 'yes':
                sys.exit(0)
        self.__get_jobs()
        __JOB_RE = re.compile(r'^Job:\s*')
        for argstr in self.__xargs_jobs(self.__filtered_jobs(args), action):
            gmjobs_out = self.__run_gmjobs('-J -S' + argstr)
            for line in iter(gmjobs_out.stdout.readline, ''):
                if __JOB_RE.match(line):
                    sys.stdout.write(line)
            sys.stdout.flush()

    def job_log(self, args):
        error_log = '{}/job.{}.errors'.format(self.control_dir, args.jobid)
        if os.path.exists(error_log):
            print_line = True
            with open(error_log, 'r') as el_f:
                for line in el_f:
                    if line.startswith('----- starting submit'):
                        print_line = args.lrms
                    if line.startswith('----- exiting submit'):
                        print_line = True
                        if not args.lrms:
                            continue
                    if print_line:
                        sys.stdout.write(line)
            sys.stdout.flush()
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
        print 'Name\t\t: {name}\n' \
              'Owner\t\t: {userdn}\n' \
              'State\t\t: {state}\n' \
              'LRMS ID\t\t: {lrmsid}\n' \
              'Modified\t: {modified}'.format(**self.jobs[args.jobid])

    def job_getattr(self, args):
        job_attrs = self.__parse_job_attrs(args.jobid)
        if args.attr:
            if args.attr in job_attrs:
                print job_attrs[args.attr]
            else:
                self.logger.error('There is no such attribute \'%s\' defined for job %s', args.attr, args.jobid)
        else:
            for k, v in job_attrs.iteritems():
                print '{:<32}: {}'.format(k, v)

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
        for line in iter(gmjobs_out.stdout.readline, ''):
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
        # show general stats per-state
        if not args.no_states:
            for s in jobstates:
                if args.long:
                    print '{state}\n  Processing: {processing:>10}\n  Waiting:    {waiting:>10}'.format(**s)
                else:
                    print '{state:>11}: {processing:>8} ({waiting})'.format(**s)
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
                    print '{state}\n  Jobs:  {jobs:>15}\n  Limit: {limit:>15}'.format(**t)
                else:
                    print '{state:>11}: {jobs:>8} of {limit}'.format(**t)
        if args.data_staging:
            if args.long:
                print 'Processing jobs in data-staging:'
                print '  Downloading: {:>9}'.format(data_download)
                print '  Uploading:   {:>9}'.format(data_upload)
            else:
                print ' Processing: {:>8} + {}'.format(data_download, data_upload)

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
        elif args.action == 'log':
            if args.service:
                self.job_service_logs(args)
            else:
                self.job_log(args)
        elif args.action == 'attr':
            self.job_getattr(args)
        elif args.action == 'stats':
            self.job_stats(args)

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
        jobs_list = jobs_actions.add_parser('list', help='List available A-REX jobs')
        jobs_list.add_argument('-l', '--long', help='Detailed listing of jobs', action='store_true')
        jobs_list.add_argument('-s', '--state', help='Filter jobs by state', action='append', choices=__JOB_STATES)
        jobs_list.add_argument('-o', '--owner', help='Filter jobs by owner').completer = complete_job_owner

        jobs_log = jobs_actions.add_parser('log', help='Display job log')
        jobs_log.add_argument('jobid', help='Job ID').completer = complete_job_id
        jobs_log.add_argument('-l', '--lrms', help='Include LRMS job submission script into the output',
                              action='store_true')
        jobs_log.add_argument('-s', '--service', help='Show ARC CE logs containing the jobID instead of job log',
                              action='store_true')

        jobs_info = jobs_actions.add_parser('info', help='Show job main info')
        jobs_info.add_argument('jobid', help='Job ID').completer = complete_job_id

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
        jobs_stats.add_argument('-S', '--no-states', help='Do not show per-state job stats', action='store_true')
        jobs_stats.add_argument('-t', '--total', help='Show server total stats', action='store_true')
        jobs_stats.add_argument('-d', '--data-staging', help='Show server datastaging stats', action='store_true')
        jobs_stats.add_argument('-l', '--long', help='Detailed output of stats', action='store_true')
