from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import sys
import os
import datetime
import subprocess
import isodate  # extra dependency on python-isodate package
import ldap
import xml.etree.ElementTree as ElementTree
import argparse
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
    return AccountingControl(arcconf).complete_vos(parsed_args)


def complete_owner(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return AccountingControl(arcconf).complete_users(parsed_args)


class AccountingControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Accounting')
        if arcconfig is None:
            self.logger.error('Failed to parse arc.conf. Jura configuration is unavailable.')
            sys.exit(1)
        self.jura_bin = ARC_LIBEXEC_DIR + '/jura'
        self.archivedir = arcconfig.get_value('archivedir', 'arex/jura/archiving')
        if self.archivedir is None:
            self.logger.warning('Accounting records achieving is not enabled! '
                                'It is not possible to operate with accounting information or doing re-publishing.')
            sys.exit(1)
        if not self.archivedir.endswith('/'):
            self.archivedir += '/'
        self.logfile = arcconfig.get_value('logfile', 'arex/jura')
        if self.logfile is None:
            self.logfile = '/var/log/arc/jura.log'
        self.ssmlog = '/var/spool/arc/ssm/ssmsend.log'  # hardcoded in JURA_DEFAULT_DIR_PREFIX and ssm/sender.cfg
        self.sgas_records = []
        self.apel_records = []

    def __xml_to_dict(self, t):
        childs = list(t)
        if childs:
            d = {t.tag: {}}
            for cd in map(self.__xml_to_dict, list(t)):
                for k, v in cd.items():
                    if k in d[t.tag]:
                        d[t.tag][k].append(v)
                    else:
                        d[t.tag][k] = [v]
        else:
            d = {t.tag: t.text}
        return d

    def __usagerecord_to_dict(self, xml_f):
        xml = ElementTree.parse(xml_f)
        return self.__xml_to_dict(xml.getroot())

    @staticmethod
    def __parse_ur_common(ardict):
        # extract common info
        arinfo = {
            'JobID': ardict['JobIdentity'][0]['GlobalJobId'][0],
            'JobName': ardict['JobName'][0],
            'Owner': None,
            'OwnerVO': None,
            'StartTime': isodate.parse_datetime(ardict['StartTime'][0]).replace(tzinfo=None),
            'EndTime': isodate.parse_datetime(ardict['EndTime'][0]).replace(tzinfo=None),
            'WallTime': isodate.parse_duration('PT0S'),
            'CPUTime': isodate.parse_duration('PT0S'),
            'Processors': 0,
        }
        # extract optional common info (possibly missing)
        if 'Processors' in ardict:
            arinfo['Processors'] = ardict['Processors'][0]
        if 'WallDuration' in ardict:
            arinfo['WallTime'] = isodate.parse_duration(ardict['WallDuration'][0])
        if 'CPUDuration' in ardict:
            for ct in ardict['WallDuration']:
                arinfo['CPUTime'] += isodate.parse_duration(ct)
        return arinfo

    def __parse_records(self, apel=True, sgas=True):
        __ns_sgas_vo = '{http://www.sgas.se/namespaces/2009/05/ur/vo}VO'
        __ns_sgas_voname = '{http://www.sgas.se/namespaces/2009/05/ur/vo}Name'
        for ar in os.listdir(self.archivedir):
            if apel and ar.startswith('usagerecordCAR.'):
                # APEL CAR records
                self.logger.debug('Processing the %s APEL accounting record', ar)
                ardict = self.__usagerecord_to_dict(self.archivedir + ar)['UsageRecord']
                # extract info
                try:
                    arinfo = self.__parse_ur_common(ardict)
                    if 'UserIdentity' in ardict:
                        if 'GlobalUserName' in ardict['UserIdentity'][0]:
                            arinfo['Owner'] = ardict['UserIdentity'][0]['GlobalUserName'][0]
                        if 'Group' in ardict['UserIdentity'][0]:
                            arinfo['OwnerVO'] = ardict['UserIdentity'][0]['Group'][0]
                except KeyError as err:
                    self.logger.error('Malformed APEL record %s found in accounting archive. Cannot find %s key.',
                                      ar, str(err))
                except IndexError as err:
                    self.logger.error('Malformed APEL record %s found in accounting archive. Error: %s.',
                                      ar, str(err))
                else:
                    self.apel_records.append(arinfo)
            elif sgas and ar.startswith('usagerecord.'):
                # SGAS accounting records
                self.logger.debug('Processing the %s SGAS accounting record', ar)
                ardict = self.__usagerecord_to_dict(self.archivedir + ar)['JobUsageRecord']
                # extract info
                try:
                    arinfo = self.__parse_ur_common(ardict)
                    if 'UserIdentity' in ardict:
                        if 'GlobalUserName' in ardict['UserIdentity'][0]:
                            arinfo['Owner'] = ardict['UserIdentity'][0]['GlobalUserName'][0]
                        if __ns_sgas_vo in ardict['UserIdentity'][0]:
                            arinfo['OwnerVO'] = ardict['UserIdentity'][0][__ns_sgas_vo][0][__ns_sgas_voname][0]
                except KeyError as err:
                    self.logger.error('Malformed SGAS record %s found in accounting archive. Cannot find %s key.',
                                      ar, str(err))
                except IndexError as err:
                    self.logger.error('Malformed SGAS record %s found in accounting archive. Error: %s.',
                                      ar, str(err))
                else:
                    self.sgas_records.append(arinfo)

    @staticmethod
    def __filter_records(records, args):
        if args.filter_vo or args.filter_user:
            frecords = []
            for r in records:
                if args.filter_vo:
                    if r['OwnerVO'] not in args.filter_vo:
                        continue
                if args.filter_user:
                    if r['Owner'] not in args.filter_user:
                        continue
                frecords.append(r)
        else:
            frecords = records
        if args.start_from:
            frecords = [x for x in frecords if (x['StartTime'] > args.start_from)]
        if args.end_till:
            frecords = [x for x in frecords if (x['EndTime'] < args.end_till)]
        return frecords

    @staticmethod
    def __get_from_till(records):
        min_from = records[0]['StartTime']
        max_till = records[0]['EndTime']
        for r in records:
            if r['StartTime'] < min_from:
                min_from = r['StartTime']
            if r['EndTime'] > max_till:
                max_till = r['EndTime']
        return min_from, max_till

    @staticmethod
    def __get_walltime(records):
        walltime = datetime.timedelta(0)
        cputime = datetime.timedelta(0)
        for r in records:
            walltime += r['WallTime']
            cputime += r['CPUTime']
        return walltime, cputime

    @staticmethod
    def __get_vos(records):
        vos = set()
        for r in records:
            vos.add(r['OwnerVO'])
        return list(vos)

    @staticmethod
    def __get_users(records):
        vos = set()
        for r in records:
            vos.add(r['Owner'])
        return list(vos)

    def __stats_show(self, records, args):
        frecords = self.__filter_records(records, args)
        if not frecords:
            self.logger.error('There are no records that match filters.')
            sys.exit(0)
        if args.jobs:
            print(len(frecords))
        elif args.walltime:
            print(self.__get_walltime(frecords)[0])
        elif args.cputime:
            print(self.__get_walltime(frecords)[1])
        elif args.vos:
            print('\n'.join(self.__get_vos(frecords)))
        elif args.users:
            print('\n'.join(self.__get_users(frecords)))
        else:
            walltime, cputime = self.__get_walltime(frecords)
            sfrom, etill = self.__get_from_till(frecords)
            kind = 'APEL' if args.apel else 'SGAS'
            jobs = len(frecords)
            print('Statistics for {0} jobs from {1} till {2}:\n' \
                  '  Number of jobs: {3:>16}\n' \
                  '  Total WallTime: {4:>16}\n' \
                  '  Total CPUTime:  {5:>16}'.format(kind, sfrom, etill, jobs, walltime, cputime))

    def stats(self, args):
        self.__parse_records(args.apel, args.sgas)
        if args.apel:
            if self.apel_records:
                self.logger.info('Showing the APEL archived records statistics')
                self.__stats_show(self.apel_records, args)
            else:
                self.logger.info('There are no APEL archived records available')
        if args.sgas:
            if self.sgas_records:
                self.logger.info('Showing the SGAS archived records statistics')
                self.__stats_show(self.sgas_records, args)
            else:
                self.logger.info('There are no SGAS archived records available')

    def republish(self, args):
        startfrom = args.start_from.strftime('%Y.%m.%d').replace('.0', '.')
        endtill = args.end_till.strftime('%Y.%m.%d').replace('.0', '.')
        command = ''
        if args.apel_url:
            command = '{0} -u {1} -t {2} -r {3}-{4} {5}'.format(
                self.jura_bin,
                args.apel_url,
                args.apel_topic,
                startfrom, endtill,
                self.archivedir
            )
        elif args.sgas_url:
            command = '{0} -u {1} -r {2}-{3} {4}'.format(
                self.jura_bin,
                args.sgas_url,
                startfrom, endtill,
                self.archivedir
            )
        self.logger.info('Running the following command to republish accounting records: %s', command)
        subprocess.call(command.split(' '))

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
            args.sgas = 'sgas' in args.type
            args.apel = 'apel' in args.type
            self.stats(args)
        elif args.action == 'logs':
            self.logs(args.ssm)
        elif args.action == 'republish':
            self.republish(args)
        elif args.action == 'apel-brokers':
            self.get_apel_brockers(args)
        else:
            self.logger.critical('Unsupported accounting action %s', args.action)
            sys.exit(1)

    def complete_vos(self, args):
        self.__parse_records()
        vos = self.__get_vos(self.__filter_records(self.sgas_records, args))
        vos += self.__get_vos(self.__filter_records(self.apel_records, args))
        return vos

    def complete_users(self, args):
        self.__parse_records()
        users = self.__get_users(self.__filter_records(self.sgas_records, args))
        users += self.__get_users(self.__filter_records(self.apel_records, args))
        return users

    @staticmethod
    def register_parser(root_parser):
        accounting_ctl = root_parser.add_parser('accounting', help='Accounting records management')
        accounting_ctl.set_defaults(handler_class=AccountingControl)

        accounting_actions = accounting_ctl.add_subparsers(title='Accounting Actions', dest='action',
                                                           metavar='ACTION', help='DESCRIPTION')

        accounting_republish = accounting_actions.add_parser('republish', help='Republish archived usage records')
        add_timeframe_args(accounting_republish, required=True)
        accounting_url = accounting_republish.add_mutually_exclusive_group(required=True)
        accounting_url.add_argument('-a', '--apel-url',
                                    help='Specify APEL server URL (e.g. https://mq.cro-ngi.hr:6163)')
        accounting_url.add_argument('-s', '--sgas-url',
                                    help='Specify APEL server URL (e.g. https://grid.uio.no:8001/logger)')
        accounting_republish.add_argument('-t', '--apel-topic', default='/queue/global.accounting.cpu.central',
                                          choices=['/queue/global.accounting.cpu.central',
                                                   '/queue/global.accounting.test.cpu.central'],
                                          help='Redefine APEL topic (default is %(default)s)')

        accounting_logs = accounting_actions.add_parser('logs', help='Show accounting logs')
        accounting_logs.add_argument('-s', '--ssm', help='Show SSM logs instead of Jura logs', action='store_true')

        accounting_list = accounting_actions.add_parser('stats', help='Show archived records statistics')
        accounting_list.add_argument('-t', '--type', help='Accounting system type',
                                     choices=['apel', 'sgas'], action='append', required=True)
        add_timeframe_args(accounting_list)
        accounting_list.add_argument('--filter-vo', help='Count only the jobs owned by this VO(s)',
                                     action='append').completer = complete_owner_vo
        accounting_list.add_argument('--filter-user', help='Count only the jobs owned by this user(s)',
                                     action='append').completer = complete_owner

        accounting_info = accounting_list.add_mutually_exclusive_group(required=False)
        accounting_info.add_argument('-j', '--jobs', help='Show number of jobs', action='store_true')
        accounting_info.add_argument('-w', '--walltime', help='Show total WallTime', action='store_true')
        accounting_info.add_argument('-c', '--cputime', help='Show total CPUTime', action='store_true')
        accounting_info.add_argument('-v', '--vos', help='Show VO that owns jobs', action='store_true')
        accounting_info.add_argument('-u', '--users', help='Show users that owns jobs', action='store_true')

        accounting_brokers = accounting_actions.add_parser('apel-brokers',
                                                           help='Fetch available APEL brokers from GLUE2 Top-BDII')
        accounting_brokers.add_argument('-t', '--top-bdii', default='ldap://lcg-bdii.cern.ch:2170',
                                        help='Top-BDII LDAP URI (default is %(default)s')
        accounting_brokers.add_argument('-s', '--ssl', help='Query for SSL brokers', action='store_true')
