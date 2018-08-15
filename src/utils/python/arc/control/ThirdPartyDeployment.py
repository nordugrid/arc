from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import sys
import requests
import socket
import ssl
import re
import subprocess
import xml.etree.ElementTree as ET
from .OSPackage import OSPackageManagement


class ThirdPartyControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.ThirdParty.Deploy')
        self.x509_cert_dir = '/etc/grid-security/certificates'
        self.arcconfig = arcconfig
        if arcconfig is None:
            self.logger.info('Failed to parse arc.conf, using default CA certificates path')
        else:
            x509_cert_dir = arcconfig.get_value('x509_cert_dir', 'common')
            if x509_cert_dir:
                self.x509_cert_dir = x509_cert_dir

    def __egi_get_voms(self, vo):
        vomslsc = {}
        self.logger.info('Fetching information about VO %s from EGI Database', vo)
        r = requests.get('http://operations-portal.egi.eu/xml/voIDCard/public/voname/{}'.format(vo))
        if not r.content.startswith('<?xml'):
            self.logger.error('VO %s is not found in EGI database')
            sys.exit(1)
        xml = ET.fromstring(r.content)
        for voms in xml.iter('VOMS_Server'):
            host = voms.find('hostname').text
            dn = voms.find('X509Cert/DN').text
            ca = voms.find('X509Cert/CA_DN').text
            vomslsc[host] = {'dn': dn, 'ca': ca}
        return vomslsc

    def __get_socket_from_url(self, url):
        port = 443
        __uri_re = re.compile(r'^(?:(?:voms|http)s?://)?(?P<host>[^:/]+)(?::(?P<port>[0-9]+))?/*.*')
        voms_re = __uri_re.match(url)
        if voms_re:
            url_params = voms_re.groupdict()
            hostname = url_params['host']
            if url_params['port'] is not None:
                port = int(url_params['port'])
        else:
            self.logger.error('Failed to parse URL %s', url)
            sys.exit(1)
        return hostname, port

    def __get_ssl_cert_openssl(self, url):
        # parse connection parameters
        (hostname, port) = self.__get_socket_from_url(url)
        # try to connect using openssl
        try:
            s_client = subprocess.Popen(['openssl', 's_client', '-connect'] + ['{}:{}'.format(hostname, port)],
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            for line in iter(s_client.stdout.readline, ''):
                if line.startswith('subject='):
                    dn = line.replace('subject=', '')
                if line.startswith('issuer='):
                    ca = line.replace('issuer=', '')
                    break
            if dn and ca:
                return {hostname: {'dn': dn.rstrip(), 'ca': ca.rstrip()}}
            self.logger.error('Failed to get DN and CA with OpenSSL SSL/TLS bind to %s:%s.', hostname, port)
            sys.exit(1)
        except OSError:
            self.logger.error('Failed to find "openssl" command. OpenSSL installation is required to use this feature.')
            sys.exit(1)

    @staticmethod
    def __x500_to_DN(attr):
        x500_map = {
            'domainComponent': 'DC',
            'countryName': 'C',
            'stateOrProvinceName': 'ST',
            'locality': 'L',
            'organizationName': 'O',
            'organizationalUnitName': 'OU',
            'commonName': 'CN'
        }
        if attr in x500_map:
            return x500_map[attr]
        return attr

    def __get_ssl_cert(self, url):
        (hostname, port) = self.__get_socket_from_url(url)
        # try to establish connection with socket/ssl directly
        try:
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            context.options |= ssl.OP_NO_SSLv2
            context.load_verify_locations(capath=self.x509_cert_dir)
            context.verify_mode = ssl.CERT_REQUIRED
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            ssl_sock = context.wrap_socket(s)
            self.logger.info('Trying to connect to %s:%s over SSL/TLS', hostname, port)
            ssl_sock.connect((hostname, port))
            self.logger.debug('Established connection to %s:%s over %s protocol', hostname, port, ssl_sock.version())
            cert = ssl_sock.getpeercert()
        except ssl.SSLError as err:
            self.logger.error('Failed to establish SSL/TLS connection to %s:%s. Error: %s', hostname, port, err.strerror)
            if 'CERTIFICATE_VERIFY_FAILED' in err.strerror:
                self.logger.error('Make sure CA certificates are deployed in %s for SSL/TLS query '
                                  'or specify correct location in arc.conf', self.x509_cert_dir)
            sys.exit(1)
        except (socket.gaierror, socket.error) as err:
            self.logger.error('Failed to establish TCP connection to %s:%s. Error: %s', hostname, port, err.strerror)
            sys.exit(1)
        except:
            self.logger.error('Unexpected error: %s', sys.exc_info()[0])
            sys.exit(1)
        if 'subject' not in cert or 'issuer' not in cert:
            self.logger.error('Failed to get DN and CA with OpenSSL SSL/TLS bind to %s:%s.', hostname, port)
            sys.exit(1)
        # convert X500 output to DN
        dn = ''
        ca = ''
        for (k, v), in cert['subject']:
            dn += '/{}={}'.format(self.__x500_to_DN(k), v)
        for (k, v), in cert['issuer']:
            ca += '/{}={}'.format(self.__x500_to_DN(k), v)
        return {hostname: {'dn': dn, 'ca': ca}}

    def lsc_deploy(self, args):
        voms_creds = {}
        # fetch VOMS server DN and CA with some of the methods
        if args.egi_vo:
            voms_creds.update(self.__egi_get_voms(args.vo))
        elif args.voms:
            if args.openssl:
                for vomsurl in args.voms:
                    voms_creds.update(self.__get_ssl_cert_openssl(vomsurl))
            else:
                for vomsurl in args.voms:
                    voms_creds.update(self.__get_ssl_cert(vomsurl))
        if not voms_creds:
            self.logger.error('There are no VOMS server credentials found. Will not create LSC file.')
            sys.exit(1)
        # create vomsdir for LSC
        vomses_dir = '/etc/grid-security/vomsdir/{}'.format(args.vo)
        if not os.path.exists(vomses_dir):
            self.logger.debug('Making vomses directory %s to hold LSC file(s)', vomses_dir)
            os.makedirs(vomses_dir, mode=0o755)
        for host, creds in voms_creds.items():
            lsc_file = '{}/{}.lsc'.format(vomses_dir, host)
            with open(lsc_file, 'w') as lsc_f:
                self.logger.info('Creating LSC file: %s', lsc_file)
                lsc_f.write('{dn}\n{ca}'.format(**creds))

    def install_cacerts_repo(self, pmobj, repo):
        if repo == 'igtf':
            # https:/dist.igtf.net/distribution/igtf/
            repoconf = {
                'yum-conf': '''[eugridpma]
name=EUGridPMA
baseurl=http://dist.eugridpma.info/distribution/igtf/current/
gpgcheck=1
gpgkey=https://dist.eugridpma.info/distribution/igtf/current/GPG-KEY-EUGridPMA-RPM-3
''',
                'yum-name': 'eugridpma.repo',
                'apt-conf': '''#### IGTF Trust Anchor Distribution ####
deb http://dist.eugridpma.info/distribution/igtf/current igtf accredited
''',
                'apt-key-url': 'https://dist.eugridpma.info/distribution/igtf/current/GPG-KEY-EUGridPMA-RPM-3',
                'apt-name': 'eugridpma.list'
            }
            pmobj.deploy_repository(repoconf)
        elif repo == 'egi-trustanchors':
            # https://wiki.egi.eu/wiki/EGI_IGTF_Release
            repoconf = {
                'apt-url': 'http://repository.egi.eu/sw/production/cas/1/current/repo-files/egi-trustanchors.list',
                'apt-key-url': 'https://dist.eugridpma.info/distribution/igtf/current/GPG-KEY-EUGridPMA-RPM-3',
                'yum-url': 'http://repository.egi.eu/sw/production/cas/1/current/repo-files/egi-trustanchors.repo'
            }
            pmobj.deploy_repository(repoconf)
        elif repo == 'nordugrid':
            print('Nordugrid repository is the general purpose repo that contains binary packages of Nordugid ARC ' \
                  'and as a bonus includes third-party packages like IGTF CA certitificates.\n' \
                  'Repositories installation depends on which version of Nordugrid ARC you want to use.\n' \
                  'Please follow the http://download.nordugrid.org/repos.html and install \'nordugrid-release\' ' \
                  'package for chosen version.\n' \
                  'If you do not want to install Nordugrid ARC packages from the nordugrid repos ' \
                  'consider the other sources of IGTF CA certificates.')
            sys.exit(0)
        else:
            self.logger.error('Unsupported CA certificates repository %s', repo)
            sys.exit(1)

    def igtf_deploy(self, bundle, installrepo):
        pm = OSPackageManagement()
        if installrepo:
            self.install_cacerts_repo(pm, installrepo)
            pm.update_cache()
        exitcode = pm.install(list(['ca_policy_igtf-' + p for p in bundle]))
        if exitcode:
            self.logger.error('Can not install IGTF CA Certificate packages. '
                              'Make sure you have repositories installed (see --help for options).')
            sys.exit(exitcode)

    def __globus_port_range(self, ports, proto, conf, iptables_config, subsys='data transfer'):
        if ports is None:
            self.logger.warning('Globus %s port range for %s is not configured! '
                                'Random port will be used which results problems with firewall.', proto, subsys)
        else:
            ports = ports.replace(',', ':').replace(' ', '')
            if conf:
                if conf['port'] == ports:
                    conf['descr'] += ' and {}'.format(subsys)
                else:
                    iptables_config.append({'descr': 'Globus {} port range for {}'.format(proto, subsys),
                                            'port': ports, 'proto': proto.lower()})
            else:
                conf.update({'descr': 'Globus {} port range for {}'.format(proto, subsys),
                        'port': ports, 'proto': proto.lower()})
                iptables_config.append(conf)

    def iptables_config(self, multiport=False, anystate=False):
        if self.arcconfig is None:
            self.logger.error('Cannot generate iptables configuration without parsed arc.conf')
            sys.exit(1)
        iptables_config = []
        # a-rex
        if self.arcconfig.check_blocks('arex/ws'):
            wsurl = self.arcconfig.get_value('wsurl', 'arex/ws')
            port = None
            if wsurl is not None:
                __uri_re = re.compile(r'^https?://(?P<host>[^:/]+)(?::(?P<port>[0-9]+))?/')
                url_match = __uri_re.match(wsurl)
                if url_match:
                    port = url_match.groupdict()['port']
                else:
                    self.logger.error('Failed to parse \'wsurl\' in [arex/ws] block. Please check the URL syntax.')
            if port is None:
                port = 443
            iptables_config.append({'descr': 'A-REX WS Interface', 'port': port})
        # a-rex data-staging
        data_staging_globus_tcp_ports = {}
        data_staging_globus_udp_ports = {}
        if self.arcconfig.check_blocks('arex/data-staging'):
            globus_tcp_ports = self.arcconfig.get_value('globus_tcp_port_range', ['arex/data-staging', 'common'])
            globus_udp_ports = self.arcconfig.get_value('globus_udp_port_range', ['arex/data-staging', 'common'])
            self.__globus_port_range(globus_tcp_ports, 'TCP', data_staging_globus_tcp_ports, iptables_config)
            self.__globus_port_range(globus_udp_ports, 'UDP', data_staging_globus_udp_ports, iptables_config)
        # gridftpd
        if self.arcconfig.check_blocks('gridftpd'):
            port = self.arcconfig.get_value('port', 'gridftpd')
            if port is None:
                port = 2811
            iptables_config.append({'descr': 'Gridftpd Interface', 'port': port})
            # globus port-ranges for gridftp
            globus_tcp_ports = self.arcconfig.get_value('globus_tcp_port_range', ['gridftpd', 'common'])
            globus_udp_ports = self.arcconfig.get_value('globus_udp_port_range', ['gridftpd', 'common'])
            self.__globus_port_range(globus_tcp_ports, 'TCP', data_staging_globus_tcp_ports, iptables_config,
                                     'gridftpd')
            self.__globus_port_range(globus_udp_ports, 'UDP', data_staging_globus_udp_ports, iptables_config,
                                     'gridftpd')
        # infosys LDAP service
        if self.arcconfig.check_blocks('infosys/ldap'):
            port = self.arcconfig.get_value('port', 'infosys/ldap')
            if port is None:
                port = 2135
            iptables_config.append({'descr': 'Infosys LDAP Service', 'port': port})
        # data-delivery service
        if self.arcconfig.check_blocks('datadelivery-service'):
            port = self.arcconfig.get_value('port', 'datadelivery-service')
            if port is None:
                port = 443
            iptables_config.append({'descr': 'Data Delivery Service', 'port': port})
        # acix-index
        if self.arcconfig.check_blocks('acix-index'):
            port = 6443  # hardcoded
            iptables_config.append({'descr': 'ACIX Index', 'port': port})
        # acix-scanner
        if self.arcconfig.check_blocks('acix-scanner'):
            port = self.arcconfig.get_value('port', 'acix-scanner')
            if port is None:
                port = 5443
            iptables_config.append({'descr': 'ACIX Scanner', 'port': port})
        # generate iptables config based on the configured services
        statestr = ' -m state --state NEW'
        if anystate:
            statestr = ''

        if multiport:
            tcp_ports = []
            udp_ports = []
            for iptc in iptables_config:
                if 'proto' not in iptc:
                    tcp_ports.append(str(iptc['port']))
                elif iptc['proto'] == 'tcp':
                    tcp_ports.append(str(iptc['port']))
                elif iptc['proto'] == 'udp':
                    udp_ports.append(str(iptc['port']))
            iptstr = '# ARC CE allowed TCP ports\n' \
                     '-A INPUT -p tcp {statestr} -m tcp -m multiport --dports {tcpports} -j ACCEPT\n' \
                     '# ARC CE allowed UDP ports\n' \
                     '-A INPUT -p udp {statestr} -m udp -m multiport --dports {udpports} -j ACCEPT'
            print(iptstr.format(statestr=statestr,
                                tcpports=','.join(tcp_ports),
                                udpports=','.join(udp_ports)))
            return

        iptstr = '# ARC CE {descr}\n-A INPUT -p {proto}' + statestr + ' -m {proto} --dport {port} -j ACCEPT'
        for iptc in iptables_config:
            if 'proto' not in iptc:
                iptc['proto'] = 'TCP'
            iptc['proto'] = iptc['proto'].lower()
            print(iptstr.format(**iptc))

    def control(self, args):
        if args.action == 'voms-lsc':
            self.lsc_deploy(args)
        elif args.action == 'igtf-ca':
            self.igtf_deploy(args.bundle, args.installrepo)
        elif args.action == 'iptables-config':
            self.iptables_config(args.multiport, args.any_state)
        else:
            self.logger.critical('Unsupported third party deployment action %s', args.action)
            sys.exit(1)

    @staticmethod
    def register_parser(root_parser):
        deploy_ctl = root_parser.add_parser('deploy', help='Third party components deployment')
        deploy_ctl.set_defaults(handler_class=ThirdPartyControl)

        deploy_actions = deploy_ctl.add_subparsers(title='Deployment Actions', dest='action',
                                                   metavar='ACTION', help='DESCRIPTION')

        deploy_voms_lsc = deploy_actions.add_parser('voms-lsc', help='Deploy VOMS list-of-certificates files')
        deploy_voms_lsc.add_argument('vo', help='VO Name')
        deploy_voms_sources = deploy_voms_lsc.add_mutually_exclusive_group(required=True)
        deploy_voms_sources.add_argument('-v', '--voms', help='VOMS-Admin URL', action='append')
        deploy_voms_sources.add_argument('-e', '--egi-vo', help='Fecth information from EGI VOs database',
                                         action='store_true')
        deploy_voms_lsc.add_argument('-o', '--openssl', action='store_true',
                                     help='Use external OpenSSL command instead of python SSL')

        igtf_ca = deploy_actions.add_parser('igtf-ca', help='Deploy IGTF CA certificates')
        igtf_ca.add_argument('bundle', help='IGTF CA bundle name', nargs='+',
                             choices=['classic', 'iota', 'mics', 'slcs'])
        igtf_ca.add_argument('-i', '--installrepo', help='Add specified repository that contains IGTF CA certificates',
                             choices=['igtf', 'egi-trustanchors', 'nordugrid'])

        iptables = deploy_actions.add_parser('iptables-config',
                                             help='Generate iptables config to allow ARC CE configured services')
        iptables.add_argument('--any-state', action='store_true',
                              help='Do not add \'--state NEW\' to filter configuration')
        iptables.add_argument('--multiport', action='store_true',
                              help='Use one-line multiport filter instead of per-service entries')
