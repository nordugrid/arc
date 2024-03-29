from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import sys
import os
try:
    from urllib.request import Request, urlopen
    from urllib.error import URLError
except ImportError:
    from urllib2 import Request, urlopen
    from urllib2 import URLError
try:
    import httplib
except ImportError:
    import http.client as httplib
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
            if arcctl_server_mode():
                self.logger.debug('Working in config-less mode. Default paths will be used.')
            else:
                self.logger.info('Failed to parse arc.conf, using default CA certificates path')
        else:
            x509_cert_dir = arcconfig.get_value('x509_cert_dir', 'common')
            if x509_cert_dir:
                self.x509_cert_dir = x509_cert_dir

    def __egi_get_voms_xml(self, vo):
        self.logger.info('Fetching information about VO %s from EGI Database', vo)
        dburl = 'http://operations-portal.egi.eu/xml/voIDCard/public/voname/{0}'.format(vo)
        # query database
        req = Request(dburl)
        try:
            self.logger.debug('Contacting EGI VO Database at %s', dburl)
            response = urlopen(req)
        except URLError as e:
            if hasattr(e, 'reason'):
                self.logger.error('Failed to reach EGI VO Database server. Error: %s', e.reason)
            else:
                self.logger.error('EGI VO Database server failed to process the request. Error code: %s', e.code)
            sys.exit(1)
        # get response
        rcontent = response.read()
        if not isinstance(rcontent, str):
            rcontent = rcontent.decode('utf-8', 'ignore')
        if not rcontent.startswith('<?xml'):
            self.logger.error('VO %s is not found in EGI database', vo)
            sys.exit(1)
        xml = ET.fromstring(rcontent)
        return xml

    def __egi_get_vomslsc(self, vo):
        vomslsc = {}
        xml = self.__egi_get_voms_xml(vo)
        # parse XML
        for voms in xml.findall('.//VOMS_Server'):
            host = voms.find('hostname').text
            dn = voms.find('X509Cert/DN').text
            ca = voms.find('X509Cert/CA_DN').text
            vomslsc[host] = {'dn': dn, 'ca': ca}
        return vomslsc

    def __egi_get_vomses(self, vo):
        vomses = []
        xml = self.__egi_get_voms_xml(vo)
        for voms in xml.findall('.//VOMS_Server'):
            port = None
            if 'VomsesPort' in voms.attrib:
                port = voms.attrib['VomsesPort']
            host = voms.find('hostname').text
            dn = voms.find('X509Cert/DN').text
            if port is not None and host and dn:
                vomses.append('"{0}" "{1}" "{2}" "{3}" "{0}"'.format(vo, host, port, dn))
        return vomses

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

    def __vomsadmin_get_vomses(self, url, vo, secure=False):
        (hostname, port) = self.__get_socket_from_url(url)
        vomsadmin_url = 'https://{0}:{1}/voms/{2}/configuration/configuration.action'.format(hostname, port, vo)
        vomsadmin_path = '/voms/{0}/configuration/configuration.action'.format(vo)

        self.logger.debug('Contacting VOMS-Admin at %s', vomsadmin_url)

        if secure:
            # try client cert
            if 'X509_USER_CERT' in os.environ and 'X509_USER_KEY' in os.environ:
                cert = os.environ['X509_USER_CERT']
                key = os.environ['X509_USER_KEY']
            else:
                cert = os.path.expanduser('~/.globus/usercert.pem')
                key = os.path.expanduser('~/.globus/userkey.pem')
                if not os.path.expanduser(key):
                    # try host cert if no user certs
                    cert = '/etc/grid-security/hostcert.pem'
                    key = '/etc/grid-security/hostkey.pem'
            if not os.path.exists(key):
                self.logger.error('Cannon find any client certificate/key installed.')
                sys.exit(1)

            conn = HTTPSClientAuthConnection(hostname, port, key, cert)
            try:
                conn.request('GET', vomsadmin_path)
                response = conn.getresponse()
            except Exception as err:
                self.logger.error('Failed to reach VOMS-Admin server. Error: %s', str(err))
                sys.exit(1)
            else:
                if response.status != 200:
                    conn.close()
                    self.logger.error('VOMS-Admin server failed to process the request. '
                                      'Error code %s (%s)', str(response.status), str(response.reason))
                    sys.exit(1)
                # get response
                rcontent = response.read()
                if not isinstance(rcontent, str):
                    rcontent = rcontent.decode('utf-8', 'ignore')
                conn.close()
        else:
            try:
                req = Request(vomsadmin_url)
                if hasattr(ssl, '_create_unverified_context'):
                    # Python 2.7.9+ has SSL verification turned on and introduce context
                    context = ssl._create_unverified_context()
                    response = urlopen(req, context=context)
                else:
                    response = urlopen(req)
            except URLError as e:
                if hasattr(e, 'reason'):
                    self.logger.error('Failed to reach VOMS-Admin server. Error: %s', e.reason)
                else:
                    self.logger.error('VOMS-Admin server failed to process the request. Error code: %s', e.code)
                self.logger.error('Try to contact VOMS-Admin with client certificate '
                                  '(using --use-client-cert option)')
                sys.exit(1)
            else:
                # get response
                rcontent = response.read()
                if not isinstance(rcontent, str):
                    rcontent = rcontent.decode('utf-8', 'ignore')

        # parse vomses from the response
        _re_vomses_header = re.compile(r'VOMSES')
        _re_vomses_content = re.compile(r'class="configurationInfo">(.*)$')
        _re_vomses_pva = re.compile(r'<p>VOMSES .*<div class="bgodd"><pre>([^<]+)')
        content_re = False
        vomses = []
        for line in rcontent.split('\n'):
            if content_re:
                _content_match = _re_vomses_content.search(line)
                if _content_match:
                    vstr = _content_match.group(1).replace('&quot;', '"')
                    vomses.append(vstr)
                    self.logger.debug('Found VOMSES string: %s', vstr)
                    break
            elif _re_vomses_header.match(line):
                content_re = True
                continue
            else:
                # PVA support
                _content_match = _re_vomses_pva.search(line)
                if _content_match:
                    self.logger.debug('Found VOMSES string: %s', _content_match.group(1))
                    vomses.append(_content_match.group(1))
                    break
        return vomses

    def __get_ssl_cert_openssl(self, url, compat=False):
        # parse connection parameters
        (hostname, port) = self.__get_socket_from_url(url)
        # try to connect using openssl
        try:
            cmd = ['openssl', 's_client', '-connect'] + ['{0}:{1}'.format(hostname, port)]
            if compat:
                cmd += ['-nameopt', 'compat']
            s_client = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            dn = None
            ca = None
            for line in iter(s_client.stdout.readline, ''):
                line = line.decode('utf-8')
                if line.startswith('subject='):
                    dn = line.replace('subject=', '')
                if line.startswith('issuer='):
                    ca = line.replace('issuer=', '')
                    break
            if dn and ca:
                if dn.startswith('/'):
                    return {hostname: {'dn': dn.rstrip(), 'ca': ca.rstrip()}}
                elif not compat:
                    self.logger.debug('Seams we are on the newer OpenSSL version, retrying with compat DN output')
                    return self.__get_ssl_cert_openssl(url, compat=True)
            self.logger.error('Failed to get DN and CA with OpenSSL SSL/TLS bind to %s:%s.', hostname, port)
            sys.exit(1)
        except OSError:
            self.logger.error('Failed to find \'openssl\' command. If OpenSSL is not installed try \'--pythonssl\' '
                              'option to fallback to native Python library.')
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
            dn += '/{0}={1}'.format(self.__x500_to_DN(k), v)
        for (k, v), in cert['issuer']:
            ca += '/{0}={1}'.format(self.__x500_to_DN(k), v)
        return {hostname: {'dn': dn, 'ca': ca}}

    def lsc_deploy(self, args):
        voms_creds = {}
        # fetch VOMS server DN and CA with some of the methods
        if args.egi_vo:
            voms_creds.update(self.__egi_get_vomslsc(args.vo))
        elif args.voms:
            if args.pythonssl:
                for vomsurl in args.voms:
                    voms_creds.update(self.__get_ssl_cert(vomsurl))
            else:
                for vomsurl in args.voms:
                    voms_creds.update(self.__get_ssl_cert_openssl(vomsurl))
        if not voms_creds:
            self.logger.error('There are no VOMS server credentials found. Will not create LSC file.')
            sys.exit(1)
        # create vomsdir for LSC
        vomsdir_dir = '/etc/grid-security/vomsdir/{0}'.format(args.vo)
        if not os.path.exists(vomsdir_dir):
            self.logger.debug('Making vomsdir directory %s to hold LSC file(s)', vomsdir_dir)
            os.makedirs(vomsdir_dir, mode=0o755)
        for host, creds in voms_creds.items():
            lsc_file = '{0}/{1}.lsc'.format(vomsdir_dir, host)
            with open(lsc_file, 'w') as lsc_f:
                self.logger.info('Creating LSC file: %s', lsc_file)
                lsc_f.write('{dn}\n{ca}'.format(**creds))

    def vomses_deploy(self, args):
        vomses = []
        # fetch VOMS server DN and CA with some of the methods
        if args.egi_vo:
            vomses = self.__egi_get_vomses(args.vo)
        elif args.voms:
            for vomsurl in args.voms:
                vomses.extend(self.__vomsadmin_get_vomses(vomsurl, args.vo, args.use_client_cert))
        if not vomses:
            self.logger.error('There are no configuration for client VOMSES fetched.')
            sys.exit(1)
        # create vomses directory
        vomses_dir = '/etc/grid-security/vomses'
        if args.user:
            vomses_dir = os.path.expanduser('~/.voms/vomses')

        if not os.path.exists(vomses_dir):
            self.logger.debug('Making vomses directory %s to hold VOMS configuration for clients', vomses_dir)
            os.makedirs(vomses_dir, mode=0o755)
        vomses_file = '{0}/{1}'.format(vomses_dir, args.vo)
        try:
            with open(vomses_file, 'w') as vomses_f:
                self.logger.info('Creating VOMSES file: %s', vomses_file)
                for vstring in vomses:
                    vomses_f.write(vstring + '\n')
        except IOError as err:
            self.logger.error('Cannot write to VOMSES file: %s (%s)', vomses_file, err)

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
        exitcode = None
        if pm.is_yum():
            exitcode = pm.install(list(['ca_policy_igtf-' + p for p in bundle]))
        elif pm.is_apt():
            exitcode = pm.install(list(['igtf-policy-' + p for p in bundle]))
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
                    conf['descr'] += ' and {0}'.format(subsys)
                else:
                    iptables_config.append({'descr': 'Globus {0} port range for {1}'.format(proto, subsys),
                                            'port': ports, 'proto': proto.lower()})
            else:
                conf.update({'descr': 'Globus {0} port range for {1}'.format(proto, subsys),
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
        elif args.action == 'vomses':
            self.vomses_deploy(args)
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
        deploy_actions.required = True

        igtf_ca = deploy_actions.add_parser('igtf-ca', help='Deploy IGTF CA certificates')
        igtf_ca.add_argument('bundle', help='IGTF CA bundle name', nargs='+',
                             choices=['classic', 'iota', 'mics', 'slcs'])
        igtf_ca.add_argument('-i', '--installrepo', help='Add specified repository that contains IGTF CA certificates',
                             choices=['igtf', 'egi-trustanchors', 'nordugrid'])

        deploy_vomses = deploy_actions.add_parser('vomses', help='Deploy VOMS client configuration files')
        deploy_vomses.add_argument('vo', help='VO Name')
        deploy_vomses_sources = deploy_vomses.add_mutually_exclusive_group(required=True)
        deploy_vomses_sources.add_argument('-v', '--voms', help='VOMS-Admin URL', action='append')
        deploy_vomses_sources.add_argument('-e', '--egi-vo', help='NOTE: BROKEN due to an EGI server change. Fetch information from EGI VOs database',
                                         action='store_true')
        deploy_vomses.add_argument('-u', '--user', help='Install to user\'s home instead of /etc',
                                   action='store_true')
        deploy_vomses.add_argument('-c', '--use-client-cert', help='Use client certificate to contact VOMS-Admin',
                                   action='store_true')

        deploy_voms_lsc = deploy_actions.add_parser('voms-lsc',
                                                    help='Deploy VOMS server-side list-of-certificates files')
        deploy_voms_lsc.add_argument('vo', help='VO Name')
        deploy_voms_sources = deploy_voms_lsc.add_mutually_exclusive_group(required=True)
        deploy_voms_sources.add_argument('-v', '--voms', help='VOMS-Admin URL', action='append')
        deploy_voms_sources.add_argument('-e', '--egi-vo', help='NOTE: BROKEN due to an EGI server change. Fetch information from EGI VOs database',
                                         action='store_true')
        deploy_voms_lsc.add_argument('--pythonssl', action='store_true',
                                     help='Use Python SSL module to establish TLS connection '
                                          '(default is to call external OpenSSL binary)')
        if arcctl_server_mode():
            iptables = deploy_actions.add_parser('iptables-config',
                                                 help='Generate iptables config to allow ARC CE configured services')
            iptables.add_argument('--any-state', action='store_true',
                                  help='Do not add \'--state NEW\' to filter configuration')
            iptables.add_argument('--multiport', action='store_true',
                                  help='Use one-line multiport filter instead of per-service entries')


class HTTPSClientAuthConnection(httplib.HTTPSConnection):
    """ Class to make a HTTPS connection, with support for full client-based SSL Authentication"""

    def __init__(self, host, port, key_file, cert_file, ca_file=None, timeout=None):
        httplib.HTTPSConnection.__init__(self, host, port, key_file=key_file, cert_file=cert_file)
        self.key_file = key_file
        self.cert_file = cert_file
        self.ca_file = ca_file
        self.timeout = timeout

    def connect(self):
        """ Connect to a host on a given (SSL) port.
            If ca_file is pointing somewhere, use it to check Server Certificate.

            Redefined/copied and extended from httplib.py:1105 (Python 2.6.x).
            This is needed to pass cert_reqs=ssl.CERT_REQUIRED as parameter to ssl.wrap_socket(),
            which forces SSL to check server certificate against our client certificate.
        """
        sock = socket.create_connection((self.host, self.port), self.timeout)
        if self._tunnel_host:
            self.sock = sock
            self._tunnel()
        # If there's no CA File, don't force Server Certificate Check
        if self.ca_file:
            self.sock = ssl.wrap_socket(sock, self.key_file, self.cert_file,
                                        ca_certs=self.ca_file, cert_reqs=ssl.CERT_REQUIRED)
        else:
            self.sock = ssl.wrap_socket(sock, self.key_file, self.cert_file,
                                        cert_reqs=ssl.CERT_NONE)
