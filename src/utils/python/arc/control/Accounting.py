from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
from .JuraArchive import JuraArchive
import sys
import os
import datetime
import subprocess
import ldap
import argparse
import tempfile
from functools import reduce


def valid_datetime_type(arg_datetime_str):
    try:
        return datetime.datetime.strptime(arg_datetime_str, "%Y-%m-%d")
    except ValueError:
        try:
            return datetime.datetime.strptime(arg_datetime_str, "%Y-%m-%d %H:%M")
        except ValueError:
            try:
                return datetime.datetime.strptime(arg_datetime_str, "%Y-%m-%d %H:%M:%S")
            except ValueError:
                msg = "Timestamp format ({0}) is not valid! " \
                      "Expected format: YYYY-MM-DD [HH:mm[:ss]]".format(arg_datetime_str)
                raise argparse.ArgumentTypeError(msg)


def add_timeframe_args(parser, required=False):
    parser.add_argument('-b', '--start-from', type=valid_datetime_type, required=required,
                        help='Limit the start time of the records (YYYY-MM-DD [HH:mm[:ss]])')
    parser.add_argument('-e', '--end-till', type=valid_datetime_type, required=required,
                        help='Limit the end time of the records (YYYY-MM-DD [HH:mm[:ss]])')


def complete_owner_vo(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_vos(prefix)


def complete_owner(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_users(prefix)


class AccountingControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Accounting')
        # runtime config
        if arcconfig is None:
            self.logger.error('Failed to parse arc.conf. Jura configuration is unavailable.')
            sys.exit(1)
        _, self.runconfig = tempfile.mkstemp(suffix='.conf', prefix='arcctl.jura.')
        self.logger.debug('Dumping runtime configuration for Jura to %s', self.runconfig)
        arcconfig.save_run_config(self.runconfig)
        # binary path include runtime config
        self.jura_bin = ARC_LIBEXEC_DIR + '/jura -c ' + self.runconfig
        # archive
        archive_dir = arcconfig.get_value('archivedir', 'arex/jura/archiving')
        if archive_dir is None:
            self.logger.warning('Accounting records archiving is not enabled! '
                                'It is not possible to operate with accounting information or doing re-publishing.')
            sys.exit(1)
        # archive manager
        accounting_db_dir = arcconfig.get_value('dbdir', 'arex/jura/archiving')
        self.archive = JuraArchive(archive_dir, accounting_db_dir)
        # logs
        self.logfile = arcconfig.get_value('logfile', 'arex/jura')
        if self.logfile is None:
            self.logfile = '/var/log/arc/jura.log'
        self.ssmlog = '/var/spool/arc/ssm/ssmsend.log'  # hardcoded in JURA_DEFAULT_DIR_PREFIX and ssm/sender.cfg
        # certificates
        x509_cert_dir = arcconfig.get_value('x509_cert_dir', 'common')
        x509_host_cert = arcconfig.get_value('x509_host_cert', 'common')
        x509_host_key = arcconfig.get_value('x509_host_key', 'common')
        if x509_cert_dir is not None:
            os.environ['X509_CERT_DIR'] = x509_cert_dir
        if x509_host_cert is not None:
            os.environ['X509_USER_CERT'] = x509_host_cert
        if x509_host_key is not None:
            os.environ['X509_USER_KEY'] = x509_host_key

    def __del__(self):
        os.unlink(self.runconfig)

    def __ensure_accounting_db(self, args):
        """Ensure accounting database availabiliy"""
        if self.archive.db_exists():
            self.archive.db_connection_init()
        elif args.db_init:
            self.logger.info('Migrating Jura archive to accounting database')
            self.archive.process_records()
        else:
            self.logger.error('Accounting database is not initialized and operation on records are not available. '
                              'Most probably jura-archive-manager is not active. '
                              'If you want to force database init from arcctl add --init-db option.')
            sys.exit(1)

    @staticmethod
    def __construct_filter(args):
        filters = {}
        if hasattr(args, 'filter_vos') and args.filter_vos:
            filters['vos'] = args.filter_vos
        if hasattr(args, 'filter_user') and args.filter_user:
            filters['owners'] = args.filter_user
        if args.start_from:
            filters['startfrom'] = args.start_from
        if args.end_till:
            filters['endtill'] = args.end_till
        return filters

    def stats(self, args):
        # construct filter
        filters = self.__construct_filter(args)
        # loop over types
        for t in args.type:
            self.logger.info('Showing the %s archived records statistics', t.upper())
            filters['type'] = t
            # show stats data (particular info requested)
            if args.jobs:
                print(self.archive.get_records_count(filters))
            elif args.walltime:
                print(self.archive.get_records_walltime(filters))
            elif args.cputime:
                print(self.archive.get_records_cputime(filters))
            elif args.vos:
                print('\n'.join(self.archive.get_records_vos(filters)))
            elif args.users:
                print('\n'.join(self.archive.get_records_owners(filters)))
            else:
                # show summary info
                count = self.archive.get_records_count(filters)
                if count:
                    sfrom, etill = self.archive.get_records_dates(filters)
                    walltime = self.archive.get_records_walltime(filters)
                    cputime = self.archive.get_records_cputime(filters)
                    print('Statistics for {0} jobs from {1} till {2}:\n'
                          '  Number of jobs: {3:>16}\n'
                          '  Total WallTime: {4:>16}\n'
                          '  Total CPUTime:  {5:>16}'.format(t.upper(), sfrom, etill, count, walltime, cputime))
                else:
                    print('There are no {0} archived records available', t.upper())

    def republish(self, args):
        if args.start_from > args.end_till:
            self.logger.error('Records start time should be before the end time.')
            sys.exit(1)
        # export necessary records to republishing directory
        filters = self.__construct_filter(args)
        filters['type'] = 'apel' if args.apel_url else 'sgas'
        exportdir = self.archive.export_records(filters)
        # define timeframe for Jura
        jura_startfrom = args.start_from.strftime('%Y.%m.%d').replace('.0', '.')
        jura_endtill = args.end_till.strftime('%Y.%m.%d').replace('.0', '.')
        command = ''
        if args.apel_url:
            command = '{0} -u {1} -t {2} -r {3}-{4} {5}'.format(
                self.jura_bin,
                args.apel_url,
                args.apel_topic,
                jura_startfrom, jura_endtill,
                exportdir
            )
        elif args.sgas_url:
            command = '{0} -u {1} -r {2}-{3} {4}'.format(
                self.jura_bin,
                args.sgas_url,
                jura_startfrom, jura_endtill,
                exportdir
            )
        self.logger.info('Running the following command to republish accounting records: %s', command)
        subprocess.call(command.split(' '))
        # clean export dir
        self.archive.export_remove()

    def get_apel_brockers(self, args):
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

    def logs(self, ssm=False):
        logfile = self.logfile
        if ssm:
            logfile = self.ssmlog
        pager_bin = 'less'
        if 'PAGER' in os.environ:
            pager_bin = os.environ['PAGER']
        p = subprocess.Popen([pager_bin, logfile])
        p.communicate()

    def control(self, args):
        if args.action == 'stats':
            self.__ensure_accounting_db(args)
            self.stats(args)
        elif args.action == 'logs':
            self.logs(args.ssm)
        elif args.action == 'republish':
            self.__ensure_accounting_db(args)
            self.republish(args)
        elif args.action == 'apel-brokers':
            self.get_apel_brockers(args)
        else:
            self.logger.critical('Unsupported accounting action %s', args.action)
            sys.exit(1)

    def complete_vos(self, prefix):
        return self.archive.get_all_vos(prefix)

    def complete_users(self, prefix):
        return self.archive.get_all_owners(prefix)

    @staticmethod
    def register_parser(root_parser):
        accounting_ctl = root_parser.add_parser('accounting', help='Accounting records management')
        accounting_ctl.set_defaults(handler_class=AccountingControl)

        accounting_actions = accounting_ctl.add_subparsers(title='Accounting Actions', dest='action',
                                                           metavar='ACTION', help='DESCRIPTION')

        # republish
        accounting_republish = accounting_actions.add_parser('republish', help='Republish archived usage records')
        add_timeframe_args(accounting_republish, required=True)
        accounting_url = accounting_republish.add_mutually_exclusive_group(required=True)
        accounting_url.add_argument('-a', '--apel-url',
                                    help='Specify APEL server URL (e.g. https://mq.cro-ngi.hr:6163)')
        accounting_url.add_argument('-s', '--sgas-url',
                                    help='Specify APEL server URL (e.g. https://grid.uio.no:8001/logger)')
        accounting_republish.add_argument('--db-init', action='store_true',
                                      help='Force accounting database init from arcctl')
        accounting_republish.add_argument('-t', '--apel-topic', default='/queue/global.accounting.cpu.central',
                                          choices=['/queue/global.accounting.cpu.central',
                                                   '/queue/global.accounting.test.cpu.central'],
                                          help='Redefine APEL topic (default is %(default)s)')

        # logs
        accounting_logs = accounting_actions.add_parser('logs', help='Show accounting logs')
        accounting_logs.add_argument('-s', '--ssm', help='Show SSM logs instead of Jura logs', action='store_true')

        # stats
        accounting_stats = accounting_actions.add_parser('stats', help='Show archived records statistics')
        accounting_stats.add_argument('-t', '--type', help='Accounting system type',
                                     choices=['apel', 'sgas'], action='append', required=True)
        add_timeframe_args(accounting_stats)
        accounting_stats.add_argument('--db-init', action='store_true',
                                      help='Force accounting database init from arcctl')
        accounting_stats.add_argument('--filter-vo', help='Count only the jobs owned by this VO(s)',
                                     action='append').completer = complete_owner_vo
        accounting_stats.add_argument('--filter-user', help='Count only the jobs owned by this user(s)',
                                     action='append').completer = complete_owner

        accounting_stats_info = accounting_stats.add_mutually_exclusive_group(required=False)
        accounting_stats_info.add_argument('-j', '--jobs', help='Show number of jobs', action='store_true')
        accounting_stats_info.add_argument('-w', '--walltime', help='Show total WallTime', action='store_true')
        accounting_stats_info.add_argument('-c', '--cputime', help='Show total CPUTime', action='store_true')
        accounting_stats_info.add_argument('-v', '--vos', help='Show VOs that owns jobs', action='store_true')
        accounting_stats_info.add_argument('-u', '--users', help='Show users that owns jobs', action='store_true')

        # apel-brockers
        accounting_brokers = accounting_actions.add_parser('apel-brokers',
                                                           help='Fetch available APEL brokers from GLUE2 Top-BDII')
        accounting_brokers.add_argument('-t', '--top-bdii', default='ldap://lcg-bdii.cern.ch:2170',
                                        help='Top-BDII LDAP URI (default is %(default)s')
        accounting_brokers.add_argument('-s', '--ssl', help='Query for SSL brokers', action='store_true')
