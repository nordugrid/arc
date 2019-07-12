from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
from .AccountingLegacy import LegacyAccountingControl
from .AccountingDB import AccountingDB, AAR
import sys
import ldap
import json
from functools import reduce


def add_timeframe_args(parser, required=False):
    parser.add_argument('-b', '--start-from', type=valid_datetime_type, required=required,
                        help='Limit the start time of the records (YYYY-MM-DD [HH:mm[:ss]])')
    parser.add_argument('-e', '--end-till', type=valid_datetime_type, required=required,
                        help='Limit the end time of the records (YYYY-MM-DD [HH:mm[:ss]])')


def complete_wlcgvo(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_wlcgvo()


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
            self.logger.error('Failed to parse arc.conf. Jura configuration is unavailable.')
            sys.exit(1)
        self.arcconfig = arcconfig
        # accounting db
        self.adb = None  # type: AccountingDB

    def __del__(self):
        if self.adb is not None:
            del self.adb
            self.adb = None

    def __init_adb(self):
        """DB connection on-demand initialization"""
        if self.adb is not None:
            return
        adb_file = self.arcconfig.get_value('controldir', 'arex').rstrip('/') + '/accounting.db'
        self.adb = AccountingDB(adb_file)

    def get_apel_brokers(self, args):
        """Fetch the list of APEL brokers from the Top-BDII"""
        try:
            ldap_conn = ldap.initialize(args.top_bdii)
            ldap_conn.protocol_version = ldap.VERSION3
            stype = 'msg.broker.stomp'
            if args.ssl:
                stype += '-ssl'
            # query GLUE2 LDAP
            self.logger.debug('Running LDAP query over %s to find %s services', args.top_bdii, stype)
            services_list = ldap_conn.search_st('o=glue', ldap.SCOPE_SUBTREE, attrlist=['GLUE2ServiceID'], timeout=30,
                                                filterstr='(&(objectClass=Glue2Service)(Glue2ServiceType={0}))'.format(
                                                    stype))
            s_ids = []
            for (_, s) in services_list:
                if 'GLUE2ServiceID' in s:
                    s_ids += s['GLUE2ServiceID']

            self.logger.debug('Running LDAP query over %s to find service endpoint URLs', args.top_bdii)
            s_filter = reduce(lambda x, y: x + '(GLUE2EndpointServiceForeignKey={0})'.format(y), s_ids, '')
            endpoints = ldap_conn.search_st('o=glue', ldap.SCOPE_SUBTREE, attrlist=['GLUE2EndpointURL'], timeout=30,
                                            filterstr='(&(objectClass=Glue2Endpoint)(|{0}))'.format(s_filter))
            for (_, e) in endpoints:
                if 'GLUE2EndpointURL' in e:
                    for url in e['GLUE2EndpointURL']:
                        print(url.replace('stomp+ssl://', 'https://').replace('stomp://', 'http://'))
            ldap_conn.unbind()
        except ldap.LDAPError as err:
            self.logger.error('Failed to query Top-BDII %s. Error: %s.', args.top_bdii, err.message['desc'])
            sys.exit(1)

    def __add_adb_filters(self, args):
        """apply optional query filters in the right order"""
        self.__init_adb()
        if args.start_from:
            self.adb.filter_startfrom(args.start_from)
        if args.end_till:
            self.adb.filter_endtill(args.end_till)
        if args.filter_user:
            self.adb.filter_users(args.filter_user)
        if args.filter_vo:
            self.adb.filter_wlcgvos(args.filter_vo)
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
        stats['rangestart'] = datetime.datetime.fromtimestamp(stats['rangestart'])
        stats['rangeend'] = datetime.datetime.fromtimestamp(stats['rangeend'])
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
            print(json.dumps(stats))

    def jobinfo(self, args):
        self.__init_adb()
        self.adb.filter_jobids([args.jobid])
        aars = self.adb.get_aars(resolve_ids=True)
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
                    print(tab + '{0}: {1}'.format(t.replace('vomsfqan', 'VOMS FQAN'), v))
            else:
                print(tab + 'There are no authentication token attributes recorded for the job.')

    def jobevents(self, jobid):
        self.__init_adb()
        self.adb.filter_jobids([jobid])
        aars = self.adb.get_aars(resolve_ids=False)
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

    def jobcontrol(self, args):
        if args.jobaction == 'info':
            self.jobinfo(args)
        elif args.jobaction == 'events':
            self.jobevents(args.jobid)
        elif args.jobaction == 'transfers':
            self.jobtransfers(args.jobid)
        else:
            self.logger.critical('Unsupported job accounting action %s', args.jobaction)
            sys.exit(1)

    def control(self, args):
        if args.action == 'stats':
            self.stats(args)
        elif args.action == 'job':
            self.jobcontrol(args)
        elif args.action == 'apel-brokers':
            self.get_apel_brokers(args)
        elif args.action == 'legacy':
            LegacyAccountingControl(self.arcconfig).control(args)
        else:
            self.logger.critical('Unsupported accounting action %s', args.action)
            sys.exit(1)

    # bash-completion helpers
    def complete_wlcgvo(self):
        self.__init_adb()
        return self.adb.get_wlcgvos()

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
    def register_parser(root_parser):
        accounting_ctl = root_parser.add_parser('accounting', help='A-REX Accounting records management')
        accounting_ctl.set_defaults(handler_class=AccountingControl)

        accounting_actions = accounting_ctl.add_subparsers(title='Accounting Actions', dest='action',
                                                           metavar='ACTION', help='DESCRIPTION')

        # add legacy accounting control as a sub-parser
        LegacyAccountingControl.register_parser(accounting_actions)

        # apel-brockers
        accounting_brokers = accounting_actions.add_parser('apel-brokers',
                                                           help='Fetch available APEL brokers from GLUE2 Top-BDII')
        accounting_brokers.add_argument('-t', '--top-bdii', default='ldap://lcg-bdii.cern.ch:2170',
                                        help='Top-BDII LDAP URI (default is %(default)s')
        accounting_brokers.add_argument('-s', '--ssl', help='Query for SSL brokers', action='store_true')

        # stats from accounting database
        accounting_stats = accounting_actions.add_parser('stats', help='Show A-REX AAR statistics')
        add_timeframe_args(accounting_stats)
        accounting_stats.add_argument('--filter-vo', help='Account jobs owned by specified WLCG VO(s)',
                                     action='append').completer = complete_wlcgvo
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
                                               'data-staged-out', 'wlcgvos', 'users', 'jobids', 'json'])

        # per-job accounting information
        job_accounting_ctl = accounting_actions.add_parser('job', help='Show job accounting data')
        AccountingControl.register_job_parser(job_accounting_ctl)
