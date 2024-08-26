from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
from .AccountingDB import AccountingDB, ACCOUNTING_DB_FILE
from .AccountingPublishing import RecordsPublisher

import sys
import json

def complete_wlcgvo(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_wlcgvo()


def complete_fqan(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_fqan()


def complete_userdn(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_userdn()


def complete_state(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_state()


def complete_queue(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_queue()


def complete_endpoint_type(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_endpoint_type()


def complete_accounting_jobid(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_jobid(prefix)


class AccountingControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Accounting')
        # arc config
        if arcconfig is None:
            self.logger.error('Failed to parse arc.conf. Accounting configuration is unavailable.')
            sys.exit(1)
        self.arcconfig = arcconfig
        # accounting db
        self.db_file = None
        self.adb = None  # type: AccountingDB

    def __del__(self):
        if self.adb is not None:
            del self.adb
            self.adb = None

    def __db_location(self, db_path):
        """Handle database default location in controldir/accounting"""
        if '/' in db_path:
            return db_path
        return os.path.join(self.arcconfig.get_value('controldir', 'arex'), "accounting", db_path)

    def __set_db_location(self, db_location=ACCOUNTING_DB_FILE):
        """Set the location of accounting database file"""
        self.db_file = self.__db_location(db_location)

    def __init_adb(self):
        """DB connection on-demand initialization"""
        if self.adb is not None:
            return
        if self.db_file is None:
            self.__set_db_location()
        self.adb = AccountingDB(self.db_file)

    def __add_adb_filters(self, args):
        """apply optional query filters in the right order"""
        self.__init_adb()
        if args.start_from:
            self.adb.filter_startfrom(args.start_from)
        if args.end_from:
            self.adb.filter_endfrom(args.end_from)
        if args.end_till:
            self.adb.filter_endtill(args.end_till)
        if args.filter_user:
            self.adb.filter_users(args.filter_user)
        if args.filter_vo:
            self.adb.filter_wlcgvos(args.filter_vo)
        if args.filter_fqan:
            self.adb.filter_fqans(args.filter_fqan)
        if args.filter_queue:
            self.adb.filter_queues(args.filter_queue)
        if args.filter_state:
            self.adb.filter_statuses(args.filter_state)
        if args.filter_endpoint:
            self.adb.filter_endpoint_type(args.filter_endpoint)
        if args.filter_extra:
            fdict = {}
            for eattr, evalue in args.filter_extra:
                if eattr not in fdict:
                    fdict[eattr] = []
                fdict[eattr].append(evalue)
            self.adb.filter_extra_attributes(fdict)

    def __human_readable(self, stats):
        """Make output human readable converting seconds and bytes to eye candy units"""
        stats['walltime'] = datetime.timedelta(seconds=stats['walltime'])
        stats['cpuusertime'] = datetime.timedelta(seconds=stats['cpuusertime'])
        stats['cpukerneltime'] = datetime.timedelta(seconds=stats['cpukerneltime'])
        stats['cputime'] = stats['cpuusertime'] + stats['cpukerneltime']
        stats['rangestart'] = datetime.datetime.utcfromtimestamp(stats['rangestart'])
        stats['rangeend'] = datetime.datetime.utcfromtimestamp(stats['rangeend'])
        stats['stagein'] = get_human_readable_size(stats['stagein'])
        stats['stageout'] = get_human_readable_size(stats['stageout'])

    def stats(self, args):
        """Print-out different aggregated accounting stats"""
        self.__init_adb()
        self.__add_adb_filters(args)
        # separate queries
        if args.output == 'users':
            out = '\n'.join(self.adb.get_job_owners())
            if out:
                print(out)
            return
        if args.output == 'jobids':
            out = '\n'.join(self.adb.get_job_ids())
            if out:
                print(out)
            return
        if args.output == 'wlcgvos':
            out = '\n'.join(self.adb.get_job_wlcgvos())
            if out:
                print(out)
            return
        if args.output == 'fqans':
            out = '\n'.join(self.adb.get_job_fqans())
            if out:
                print(out)
            return
        # common stats query for other options
        stats = self.adb.get_stats()
        if stats['count'] == 0:
            self.logger.error('There are no jobs matching defined filtering criteria')
        if args.output == 'brief' and stats['count']:
            self.__human_readable(stats)
            print('A-REX Accounting Statistics:\n'
                  '  Number of Jobs: {count}\n'
                  '  Execution timeframe: {rangestart} - {rangeend}\n'
                  '  Total WallTime: {walltime}\n'
                  '  Total CPUTime: {cputime} (including {cpukerneltime} of kernel time)\n'
                  '  Data staged in: {stagein}\n'
                  '  Data staged out: {stageout}'.format(**stats))
        elif args.output == 'jobcount':
            print(stats['count'])
        elif args.output == 'walltime':
            print(stats['walltime'])
        elif args.output == 'cputime':
            print(stats['cpuusertime'] + stats['cpukerneltime'])
        elif args.output == 'data-staged-in':
            print(stats['stagein'])
        elif args.output == 'data-staged-out':
            print(stats['stageout'])
        elif args.output == 'json':
            stats['cputime'] = stats['cpuusertime'] + stats['cpukerneltime']
            stats['users'] = self.adb.get_job_owners()
            stats['wlcgvos'] = self.adb.get_job_wlcgvos()
            stats['fqans'] = self.adb.get_job_fqans()
            print(json.dumps(stats))

    def jobinfo(self, args):
        self.__init_adb()
        self.adb.filter_jobids([args.jobid])
        aars = self.adb.get_aars(resolve_ids=True)
        if not aars:
            self.logger.error('There are no job accounting information found for job %s', args.jobid)
            sys.exit(1)
        # jobinfo only works with a single job
        aar = aars[0]
        # no accounting info for in-progress jobs
        if aar.status() == 'in-progress':
            print('Job {JobID} (submitted on {SubmitTime}) is not finished yet. '
                  'Accounting data is not available yet.'.format(**aar.get()))
            return
        # json output dump entire structure as it is
        if args.output == 'json':
            self.adb.enrich_aars(aars, True, True, True, True, True)
            print(json.dumps(aar.get(), default=str))
            return
        # human readable sizes for output
        aar.add_human_readable()
        # all info output requires extra headers, etc
        all = True if args.output == 'all' else False
        tab = ''
        if all:
            # job info header
            header = 'Job {0} accounting info:'.format(args.jobid)
            print(header)
            print('='*len(header))
            tab = '  '
        # different kinds of info
        if args.output == 'description' or all:
            # general info
            voinfo = 'with no WLCG VO affiliation'
            if aar.wlcgvo():
                voinfo = 'as a member of "{WLCGVO}" WLCG VO'
                if aar.fqan():
                    voinfo += ' ({FQAN})'
            if all:
                print('Job description:')
            description = tab + \
                'Job was submitted at {SubmitTime} via "{Interface}" interface using "{EndpointURL}" endpoint.\n' + \
                tab + 'Job owned by "{UserSN}" ' + voinfo + '.\n' + \
                tab + 'It was targeted to the "{Queue}" queue with "{LocalJobID}" LRMS ID.\n' + \
                tab + 'Job {Status} with exit code {ExitCode} at {EndTime}.'
            print(description.format(**aar.get()))
            # extra attributes
            self.adb.enrich_aars(aars, extra=True)
            extrainfo = aar.extra()
            if extrainfo:
                print('  Following job properties are recorded:')
            for attr in extrainfo.keys():
                print('    {0}: {1}'.format(attr.capitalize(), extrainfo[attr]))
            # benchmark in v2
            if self.adb.db_version >= 2:
                print('    BENCHMARK: {0}'.format(aar.get()['Benchmark']))
        if args.output == 'resources' or all:
            # resource usage
            if all:
                print('Resource usage:')
            resources = tab + 'Execution timeframe: {SubmitTime} - {EndTime}\n' + \
                  tab + 'Used WallTime: {UsedWalltime}\n' + \
                  tab + 'Used CPUTime: {UsedCPUTime} (including {UsedCPUKernelTime} of kernel time)\n' + \
                  tab + 'Used WN Scratch: {UsedScratchHR}\n' + \
                  tab + 'Max physical memory: {UsedMemoryHR}\n' + \
                  tab + 'Max virtual memory: {UsedVirtMemHR}\n' + \
                  tab + 'Used CPUs: {CPUCount} on {NodeCount} node(s)\n' + \
                  tab + 'Data staged in: {StageInVolumeHR}\n' + \
                  tab + 'Data staged out: {StageOutVolumeHR}'
            print(resources.format(**aar.get()))
        if args.output == 'rtes' or all:
            if all:
                print('Used RunTime Environments:')
            self.adb.enrich_aars(aars, rtes=True)
            rtes = aar.rtes()
            rtesstr = ('\n' + tab).join(rtes)
            if rtesstr:
                print(tab + rtesstr)
            else:
                print(tab + 'There are no RTEs used by the job.')
        if args.output == 'authtokens' or all:
            if all:
                print('Auth token attributes provided:')
            self.adb.enrich_aars(aars, authtokens=True)
            authtokens = aar.authtokens()
            if authtokens:
                for (t, v) in authtokens:
                    if t == 'vomsfqan' or t == 'mainfqan':
                        t = 'VOMS FQAN'
                    print(tab + '{0}: {1}'.format(t, v))
            else:
                print(tab + 'There are no authentication token attributes recorded for the job.')

    def jobevents(self, jobid):
        self.__init_adb()
        self.adb.filter_jobids([jobid])
        aars = self.adb.get_aars(resolve_ids=False)
        if not aars:
            self.logger.error('There are no job accounting information found for job %s', jobid)
            sys.exit(1)
        self.adb.enrich_aars(aars, events=True)
        events = aars[0].events()
        if not events:
            self.logger.error('There are no events registered for job %s', jobid)
            return
        for (event, date) in events:
            print('{0}\t{1}'.format(date, event))

    def jobtransfers(self, jobid):
        self.__init_adb()
        self.adb.filter_jobids([jobid])
        aars = self.adb.get_aars(resolve_ids=False)
        if not aars:
            self.logger.error('There are no job accounting information found for job %s', jobid)
            sys.exit(1)
        self.adb.enrich_aars(aars, dtrs=True)
        datatransfers = aars[0].datatransfers()
        if not datatransfers:
            self.logger.error('There are no data transfers registered for job %s', jobid)
            return
        # sort by transfer type
        sorteddtr = {
            'input': [],
            'output': []
        }
        for dtrinfo in datatransfers:
            if dtrinfo['type'] == 'output':
                sorteddtr['output'].append(dtrinfo)
            else:
                sorteddtr['input'].append(dtrinfo)
        # stage-in
        if sorteddtr['input']:
            print('Data transfers (downloads) performed during A-REX stage-in:')
            for dtr in sorteddtr['input']:
                fromcache = ' (from cache)' if dtr['type'] == 'cache_input' else ''
                print('  {0}{1}:\n    Size: {2}\n    Download timeframe: {3} - {4}'.format(
                    dtr['url'], fromcache, get_human_readable_size(dtr['size']),
                    dtr['timestart'], dtr['timeend']
                ))
        else:
            print('No stage-in data transfers (downloads) performed by A-REX.')
        # stage-out
        if sorteddtr['output']:
            print('Data transfers (uploads) performed during A-REX stage-out:')
            for dtr in sorteddtr['output']:
                print('  {0}:\n    Size: {1}\n    Upload timeframe: {2} - {3}'.format(
                    dtr['url'], get_human_readable_size(dtr['size']),
                    dtr['timestart'], dtr['timeend']
                ))
        else:
            print('No stage-out data transfers (uploads) performed by A-REX.')

    def __add_apel_options(self, args, targetconf, required=False):
        # topic is mandatory and default exists
        if args.apel_topic is not None:
            targetconf['topic'] = args.apel_topic
        elif required:
            targetconf['topic'] = 'gLite-APEL'
        # message type to send is mandatory and default exists
        if args.apel_messages is not None:
            targetconf['apel_messages'] = args.apel_messages
        elif required:
            targetconf['apel_messages'] = 'summaries'
        # configurable APEL project
        if args.apel_project is not None:
            targetconf['project'] = args.apel_project
        elif required:
            targetconf['project'] = 'accounting'
        # gocdb is mandatory (if not specified, check will fail in __check_target_confdict during republish)
        if args.gocdb_name is not None:
            targetconf['gocdb_name'] = args.gocdb_name
        # vofilter is optional
        if args.vofilter is not None:
            targetconf['vofilter'] = args.vofilter
        # urbatchsize has a default
        if args.urbatchsize is not None:
            targetconf['urbatchsize'] = args.urbatchsize
        elif required:
            targetconf['urbatchsize'] = 500

    def __add_sgas_options(self, args, targetconf, required=False):
        # localid_prefix is optional
        if args.localid_prefix is not None:
            targetconf['localid_prefix'] = args.localid_prefix
        # vofilter is optional
        if args.vofilter is not None:
            targetconf['vofilter'] = args.vofilter
        # urbatchsize has a default
        if args.urbatchsize is not None:
            targetconf['urbatchsize'] = args.urbatchsize
        elif required:
            targetconf['urbatchsize'] = 50

    def republish(self, args):
        targetconf = {}
        targettype = None
        targetid = None
        confrequired = True
        self.__init_adb()
        publisher = RecordsPublisher(self.arcconfig, self.adb)
        if args.end_from > args.end_till:
            self.logger.error('Republishing timeframe specified incorrectly: "--end-from" cannot be later that "--end-till"')
            sys.exit(1)
        if args.target_name is not None:
            targetid = args.target_name
            (targetconf, targettype) = publisher.find_configured_target(targetid)
            if targetconf is None:
                self.logger.error('Failed to find target %s configuration in the arc.conf to do republishing.',
                                  targetid)
                sys.exit(1)
            confrequired = False
        elif args.apel_url is not None:
            targetid = args.apel_url
            targettype = 'apel'
            targetconf['targeturl'] = targetid
        elif args.sgas_url is not None:
            targetid = args.sgas_url
            targettype = 'sgas'
            targetconf['targeturl'] = targetid
        # create or redefine target configuration
        if targettype == 'apel':
            self.__add_apel_options(args, targetconf, required=confrequired)
        elif targettype == 'sgas':
            self.__add_sgas_options(args, targetconf, required=confrequired)
        # run republishing
        if targetconf:
            if not publisher.republish(targettype, targetconf, args.end_from, args.end_till, args.dry_run):
                self.logger.error('Failed to republish accounting data from {0} till {1} to {2} target {3}'.format(
                    args.end_from, args.end_till, targettype.upper(), targetid
                ))
                sys.exit(1)
        if not args.dry_run:
            self.logger.info('Accounting data from {0} till {1} to {2} target {3} has been republished.'.format(
                args.end_from, args.end_till, targettype.upper(), targetid
            ))

    def backup(self, location):
        self.__init_adb()
        # TODO: check there is enough space
        self.logger.info('Starting accounting database backup to %s', location)
        backup_adb = AccountingDB(location, must_exists=False)
        self.adb.backup(backup_adb)

    def db_migrate(self, args):
        self.__init_adb()
        adb_v1_file = self.__db_location(args.from_db)
        adb_v1 = AccountingDB(adb_v1_file)
        if adb_v1.adb_version() != 1:
            self.logger.error('Migration only works from database schema v1. Database at %s has schema version %s.',
                              adb_v1_file, adb_v1.adb_version())
            sys.exit(1)
        self.logger.info('Migrate records from %s for jobs finished after %s', adb_v1_file, str(args.end_from))
        adb_v1.filter_endfrom(args.end_from)
        if args.end_till:
            adb_v1.filter_endtill(args.end_till)
            self.logger.info('Only jobs finished before %s will be migrated', str(args.end_till))
        self.logger.error('Not implemeneted yet')
        sys.exit(1)

    def jobcontrol(self, args):
        canonicalize_args_jobid(args)
        if args.jobaction == 'info':
            self.jobinfo(args)
        elif args.jobaction == 'events':
            self.jobevents(args.jobid)
        elif args.jobaction == 'transfers':
            self.jobtransfers(args.jobid)
        else:
            self.logger.critical('Unsupported job accounting action %s', args.jobaction)
            sys.exit(1)

    def dbcontrol(self, args):
        if args.dbaction == 'backup':
            self.backup(args.location)
        elif args.dbaction == 'migrate':
            pass
        elif args.dbaction == 'cleanup':
            pass
        elif args.dbaction == 'rotate':
            pass
        else:
            self.logger.critical('Unsupported accounting database action %s', args.dbaction)
            sys.exit(1)

    def control(self, args):
        # optional custom database location (e.g. backup copy)
        if args.database_file:
            self.__set_db_location(args.database_file)
        # action-based routines
        if args.action == 'stats':
            self.stats(args)
        elif args.action == 'database' or 'dbaction' in args:
            self.dbcontrol(args)
        elif args.action == 'job' or 'jobaction' in args:
            self.jobcontrol(args)
        elif args.action == 'republish':
            self.republish(args)
        else:
            self.logger.critical('Unsupported accounting action %s', args.action)
            sys.exit(1)

    # bash-completion helpers
    def complete_wlcgvo(self):
        self.__init_adb()
        return self.adb.get_wlcgvos()

    def complete_fqan(self):
        self.__init_adb()
        return self.adb.get_fqans()

    def complete_userdn(self):
        self.__init_adb()
        return self.adb.get_users()

    def complete_state(self):
        self.__init_adb()
        return self.adb.get_statuses()

    def complete_queue(self):
        self.__init_adb()
        return self.adb.get_queues()

    def complete_endpoint_type(self):
        self.__init_adb()
        return self.adb.get_endpoint_types()

    def complete_jobid(self, prefix):
        self.__init_adb()
        return self.adb.get_joblist(prefix)

    @staticmethod
    def register_job_parser(job_accounting_ctl):
        job_accounting_ctl.set_defaults(handler_class=AccountingControl)

        accounting_actions = job_accounting_ctl.add_subparsers(title='Job Accounting Actions', dest='jobaction',
                                                               metavar='ACTION', help='DESCRIPTION')
        accounting_actions.required = True

        accounting_job = accounting_actions.add_parser('info', help='Show job accounting data')
        accounting_job.add_argument('-o', '--output', default='all',
                                      help='Define what kind of job information you want to output '
                                           '(default is %(default)s)',
                                      choices=['all', 'description', 'resources', 'rtes', 'authtokens', 'json'])
        accounting_job.add_argument('jobid', help='Job ID').completer = complete_accounting_jobid

        accounting_events = accounting_actions.add_parser('events', help='Show job event history')
        accounting_events.add_argument('jobid', help='Job ID').completer = complete_accounting_jobid

        accounting_dtr = accounting_actions.add_parser('transfers', help='Show job data transfers statistics')
        accounting_dtr.add_argument('jobid', help='Job ID').completer = complete_accounting_jobid

    @staticmethod
    def register_db_parser(job_db_ctl):
        job_db_ctl.set_defaults(handler_class=AccountingControl)

        db_actions = job_db_ctl.add_subparsers(title='Accounting Database Actions', dest='dbaction',
                                               metavar='ACTION', help='DESCRIPTION')
        db_actions.required = True
        db_backup = db_actions.add_parser('backup', help='Run online backup of accounting database')
        db_backup.add_argument('--location', help='Database backup file path', required=True)

        # TODO: implement
        db_migrate = db_actions.add_parser('migrate', help='Migrate records from legacy ARC6 database')
        db_migrate.add_argument('-f', '--from-db', help='Legacy database file (default is %(default)s)',
                                default='accounting.db', required=True)
        db_migrate.add_argument('-b', '--end-from', type=valid_datetime_type, required=True,
                                help='Define from which point in time migrate records (YYYY-MM-DD [HH:mm[:ss]])')
        db_migrate.add_argument('-e', '--end-till', type=valid_datetime_type, required=False,
                                help='Optionally define to which point in time migrate records (YYYY-MM-DD [HH:mm[:ss]]).')

        db_cleanup = db_actions.add_parser('cleanup', help='Delete old records from accounting database')
        db_cleanup.add_argument('-e', '--end-till', type=valid_datetime_type, required=True,
                                help='Cleanup records older than specified completion time (YYYY-MM-DD [HH:mm[:ss]])')
        db_cleanup.add_argument('-b', '--end-from', type=valid_datetime_type, required=False,
                                help='Optionally define from which point in time cleanup records (YYYY-MM-DD [HH:mm[:ss]]).')
        db_cleanup.add_argument('--vacuum', help='Vacuum database (this is blocking action)', action='store_true')

        db_rotate = db_actions.add_parser('rotate', help='Rotate accounting database')
        db_rotate.add_argument('--date', type=valid_datetime_yymmdd, required=True,
                                help='Rotate database at specified date (YYYY-MM-DD)')
        db_rotate.add_argument('--vacuum', help='Vacuum database (this is blocking action)', action='store_true')

    @staticmethod
    def register_parser(root_parser):
        accounting_ctl = root_parser.add_parser('accounting', help='A-REX Accounting records management')
        accounting_ctl.set_defaults(handler_class=AccountingControl)
        accounting_ctl.add_argument('--database-file', help='Set custom location of accounting database to work with. Path or file name inside controldir.')

        accounting_actions = accounting_ctl.add_subparsers(title='Accounting Actions', dest='action',
                                                           metavar='ACTION', help='DESCRIPTION')
        accounting_actions.required = True

        # stats from accounting database
        accounting_stats = accounting_actions.add_parser('stats', help='Show A-REX AAR statistics')
        accounting_stats.add_argument('-b', '--end-from', type=valid_datetime_type,
                                      help='Define the job completion time range beginning (YYYY-MM-DD [HH:mm[:ss]])')
        accounting_stats.add_argument('-e', '--end-till', type=valid_datetime_type,
                                      help='Define the job completion time range end (YYYY-MM-DD [HH:mm[:ss]])')
        accounting_stats.add_argument('-s', '--start-from', type=valid_datetime_type,
                                      help='Define the job start time constraint (YYYY-MM-DD [HH:mm[:ss]])')
        accounting_stats.add_argument('--filter-vo', help='Account jobs owned by specified WLCG VO(s)',
                                     action='append').completer = complete_wlcgvo
        accounting_stats.add_argument('--filter-fqan', help='Account jobs owned by users with specified FQAN(s)',
                                     action='append').completer = complete_fqan
        accounting_stats.add_argument('--filter-user', help='Account jobs owned by specified user(s)',
                                     action='append').completer = complete_userdn
        accounting_stats.add_argument('--filter-state', help='Account jobs in the defined state(s)',
                                      action='append').completer = complete_state
        accounting_stats.add_argument('--filter-queue', help='Account jobs submitted to the defined queue(s)',
                                      action='append').completer = complete_queue
        accounting_stats.add_argument('--filter-endpoint', help='Account jobs submitted via defined endpoint type(s)',
                                      action='append').completer = complete_endpoint_type
        accounting_stats.add_argument('--filter-extra', nargs=2, action='append', metavar=('ATTRIBUTE', 'VALUE'),
                                      help='Filter extra attributes (e.g. jobname, project, vomsfqan, rte, dtrurl, etc)'
                                      )

        accounting_stats.add_argument('-o', '--output', default='brief',
                                      help='Define what kind of stats you want to output (default is %(default)s)',
                                      choices=['brief', 'jobcount', 'walltime', 'cputime', 'data-staged-in',
                                               'data-staged-out', 'wlcgvos', 'fqans', 'users', 'jobids', 'json'])

        # per-job accounting information
        job_accounting_ctl = accounting_actions.add_parser('job', help='Show job accounting data')
        AccountingControl.register_job_parser(job_accounting_ctl)

        # database-level operations
        db_accounting_ctl = accounting_actions.add_parser('database', help='Accounting database operations')
        AccountingControl.register_job_parser(db_accounting_ctl)

        # republish
        accounting_republish = accounting_actions.add_parser('republish',
                                                             help='Republish accounting records to defined target')
        accounting_republish.add_argument('--dry-run', help='Print out messages instead of sending them to target', action='store_true')
        accounting_republish.add_argument('-b', '--end-from', type=valid_datetime_type, required=True,
                                          help='Define republishing timeframe start (YYYY-MM-DD [HH:mm[:ss]])')
        accounting_republish.add_argument('-e', '--end-till', type=valid_datetime_type, required=True,
                                          help='Define republishing timeframe end (YYYY-MM-DD [HH:mm[:ss]])')

        accounting_target = accounting_republish.add_mutually_exclusive_group(required=True)
        accounting_target.add_argument('-t', '--target-name',
                                       help='Specify configured accounting target name from arc.conf (e.g. neic_sgas).')
        accounting_target.add_argument('-a', '--apel-url',
                                       help='Specify APEL server URL (e.g. https://msg.argo.grnet.gr)')
        accounting_target.add_argument('-s', '--sgas-url',
                                       help='Specify SGAS server URL (e.g. https://grid.uio.no:8001/logger)')

        accounting_republish.add_argument_group(title='Configured Target',
                                  description='For arc.conf targets (--target-name) all options are initially fetched '
                                              'from the configuration. Target-specific options defined on the '
                                              'command line can be used to override the arc.conf values.')

        apel_options = accounting_republish.add_argument_group(title='APEL',
                                  description='Options to be used when target is specified using --apel-url')
        apel_options.add_argument('--apel-topic', required=False,
                                  help='Define APEL topic (default is gLite-APEL)',
                                  choices=['gLite-APEL', '/queue/global.accounting.test.cpu.central'])
        apel_options.add_argument('--apel-messages', required=False,
                                  help='Define APEL messages (default is summaries)',
                                  choices=['urs', 'summaries', 'summaries-v04'])
        apel_options.add_argument('--apel-project', required=False, help='Define APEL project (default is "accounting")')
        apel_options.add_argument('--gocdb-name', required=False, help='(Re)define GOCDB site name')

        sgas_options = accounting_republish.add_argument_group(title='SGAS',
                                  description='Options to be used when target is specified using --sgas-url')
        sgas_options.add_argument('--localid-prefix', required=False, help='Define optional SGAS localid prefix')

        common_options = accounting_republish.add_argument_group(title='Other options',
                                                                 description='Works for both APEL and SGAS targets')
        common_options.add_argument('--vofilter', required=False, action='append',
                                    help='Republish only jobs owned by these VOs')
        common_options.add_argument('--urbatchsize', required=False, help='Size of records batch to be send '
                                                                          '(default is 50 for SGAS, 500 for APEL)')
