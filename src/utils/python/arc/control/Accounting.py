from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
from .AccountingLegacy import LegacyAccountingControl
import sys
import ldap
from functools import reduce


def add_timeframe_args(parser, required=False):
    parser.add_argument('-b', '--start-from', type=valid_datetime_type, required=required,
                        help='Limit the start time of the records (YYYY-MM-DD [HH:mm[:ss]])')
    parser.add_argument('-e', '--end-till', type=valid_datetime_type, required=required,
                        help='Limit the end time of the records (YYYY-MM-DD [HH:mm[:ss]])')


class AccountingControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Accounting')
        # arc config
        if arcconfig is None:
            self.logger.error('Failed to parse arc.conf. Jura configuration is unavailable.')
            sys.exit(1)
        self.arcconfig = arcconfig

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

    def control(self, args):
        if args.action == 'apel-brokers':
            self.get_apel_brockers(args)
        elif args.action == 'legacy':
            LegacyAccountingControl(self.arcconfig).control(args)
        else:
            self.logger.critical('Unsupported accounting action %s', args.action)
            sys.exit(1)

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
