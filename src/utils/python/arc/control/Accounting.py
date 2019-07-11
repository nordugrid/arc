from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
from .AccountingLegacy import LegacyAccountingControl
from .AccountingDB import AccountingDB
import sys
import ldap
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

    # not every accounting subsystem operation requires db connection, so init is on-demand
    def __init_adb(self):
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

    def __human_readable(self, stats):
        stats['walltime'] = datetime.timedelta(seconds=stats['walltime'])
        stats['cpuusertime'] = datetime.timedelta(seconds=stats['cpuusertime'])
        stats['cpukerneltime'] = datetime.timedelta(seconds=stats['cpukerneltime'])
        stats['cputime'] = stats['cpuusertime'] + stats['cpukerneltime']
        stats['rangestart'] = datetime.datetime.fromtimestamp(stats['rangestart'])
        stats['rangeend'] = datetime.datetime.fromtimestamp(stats['rangeend'])
        stats['stagein'] = get_human_readable_size(stats['stagein'])
        stats['stageout'] = get_human_readable_size(stats['stageout'])

    def stats(self, args):
        self.__init_adb()
        self.__add_adb_filters(args)
        stats = self.adb.get_stats()
        if args.output == 'brief':
            self.__human_readable(stats)
            print('A-REX accounting statistics from {rangestart} till {rangeend}:\n'
                  '  Number of jobs: {count}\n'
                  '  Total WallTime: {walltime}\n'
                  '  Total CPUTime: {cputime} (including {cpukerneltime} of kernel time)\n'
                  '  Total data staged in: {stagein}\n'
                  '  Total data staged out: {stageout}'.format(**stats))

    def control(self, args):
        if args.action == 'stats':
            self.stats(args)
        elif args.action == 'apel-brokers':
            self.get_apel_brokers(args)
        elif args.action == 'legacy':
            LegacyAccountingControl(self.arcconfig).control(args)
        else:
            self.logger.critical('Unsupported accounting action %s', args.action)
            sys.exit(1)

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

        accounting_stats.add_argument('-o', '--output', default='brief',
                                      help='Define what kind of stats you want to output (default is %(default)s)',
                                      choices=['brief', 'jobcount', 'walltime', 'cputime', 'data-staged-in',
                                               'data-staged-out', 'wlcgvos', 'users', 'json'])
