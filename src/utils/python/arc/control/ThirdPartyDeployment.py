from ControlCommon import *
import sys
import requests
import socket
import ssl
import re
import subprocess
import xml.etree.ElementTree as ET


class ThirdPartyControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.ThirdParty.Deploy')
        self.x509_cert_dir = '/etc/grid-security/certificates'
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
            subprocess.call(['openssl', 's_client', '-connect'] + ['{}:{}'.format(hostname, port)])
            # TODO: parse output for DN
        except OSError:
            self.logger.error('Failed to find "openssl" command. OpenSSL installation is required to use this feature.')

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
        # TODO: convert cert dict to DN

    def lsc_deploy(self, args):
        if args.egi_vo:
            print self.__egi_get_voms(args.vo)
        elif args.voms:
            if args.openssl:
                for vomsurl in args.voms:
                    self.__get_ssl_cert_openssl(vomsurl)
            else:
                for vomsurl in args.voms:
                    self.__get_ssl_cert(vomsurl)

    def enable_cacerts_repo(self, repotype):
        # Detect apt vs yum
        # TODO: http://repository.egi.eu/sw/production/cas/1/current/repo-files/
        # TODO: https://dist.igtf.net/distribution/igtf/
        # TODO: Nordugrid-Repo (suggest to follow URL, thus there is to much versioning)
        pass

    def control(self, args):
        if args.action == 'voms-lsc':
            self.lsc_deploy(args)
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
        deploy_voms_sources.add_argument_group('voms-admin', 'voms-admin certificate query')
        deploy_voms_sources.add_argument('-v', '--voms', nargs='*', help='VOMS-Admin URL')
        deploy_voms_sources.add_argument('-e', '--egi-vo', help='Fecth information from EGI VOs database',
                                         action='store_true')
        deploy_voms_lsc.add_argument('-o', '--openssl', action='store_true',
                                     help='Use external OpenSSL command instead of native python SSL')

