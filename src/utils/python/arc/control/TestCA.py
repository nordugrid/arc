from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
from .CertificateGenerator import CertificateGenerator, CertificateKeyPair
import socket
import sys
import stat
import tempfile
import shutil
import datetime
import tarfile


def add_parser_digest_validity(parser, defvalidity=90):
    parser.add_argument('-d', '--digest', help='Digest to use (default is %(default)s)', default='sha256',
                        choices=['md2', 'md4', 'md5', 'mdc2', 'sha1', 'sha224', 'sha256', 'sha384', 'sha512'])
    parser.add_argument('-v', '--validity', type=int, default=defvalidity,
                        help='Validity of certificate in days (default is %(default)s)')


class TestCAControl(ComponentControl):
    __test_hostcert = '/etc/grid-security/testCA-hostcert.pem'
    __test_hostkey = '/etc/grid-security/testCA-hostkey.pem'

    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.TestCA')
        self.x509_cert_dir = '/etc/grid-security/certificates'
        self.arcconfig = arcconfig
        if arcconfig is None:
            self.logger.info('Failed to parse arc.conf, using default CA certificates directory')
        else:
            x509_cert_dir = arcconfig.get_value('x509_cert_dir', 'common')
            if x509_cert_dir:
                self.x509_cert_dir = x509_cert_dir
        self.hostname = socket.gethostname()
        self.caName = 'ARC {0} TestCA'.format(self.hostname)
        self.caKey = os.path.join(self.x509_cert_dir, self.caName.replace(' ', '-') + '-key.pem')
        self.caCert = os.path.join(self.x509_cert_dir, self.caName.replace(' ', '-') + '.pem')

    def createca(self, args):
        # CA certificates dir
        if not os.path.exists(self.x509_cert_dir):
            self.logger.debug('Making CA certificates directory at %s', self.x509_cert_dir)
            os.makedirs(self.x509_cert_dir, mode=0o755)
        # CA name from hostname
        cg = CertificateGenerator(self.x509_cert_dir)
        cg.generateCA(self.caName, validityperiod=args.validity, messagedigest=args.digest, force=args.force)

    def signhostcert(self, args):
        ca = CertificateKeyPair(self.caKey, self.caCert)
        tmpdir = tempfile.mkdtemp()
        cg = CertificateGenerator(tmpdir)
        hostname = self.hostname if args.hostname is None else args.hostname
        hostcertfiles = cg.generateHostCertificate(hostname, ca=ca, validityperiod=args.validity,
                                                   messagedigest=args.digest)
        if args.hostname is not None:
            workdir = os.getcwd()
            certfname = hostcertfiles.certLocation.split('/')[-1]
            keyfname = hostcertfiles.keyLocation.split('/')[-1]
            if not args.force:
                if os.path.exists(os.path.join(workdir, certfname)) or os.path.exists(os.path.join(workdir, keyfname)):
                    logger.error('Host certificate for %s is already exists.', hostname)
                    shutil.rmtree(tmpdir)
                    sys.exit(1)
            os.chmod(os.path.join(workdir, keyfname), stat.S_IRUSR | stat.S_IWUSR)
            shutil.move(hostcertfiles.certLocation, os.path.join(workdir, certfname))
            shutil.move(hostcertfiles.keyLocation, os.path.join(workdir, keyfname))
            print('Host certificate and key are saved to {0} and {1} respectively.'.format(certfname, keyfname))
        else:
            if not args.force:
                if os.path.exists(self.__test_hostcert) or os.path.exists(self.__test_hostkey):
                    logger.error('Host certificate is already exists.')
                    shutil.rmtree(tmpdir)
                    sys.exit(1)
            logger.info('Installing generated host certificate to %s', self.__test_hostcert)
            shutil.move(hostcertfiles.certLocation, self.__test_hostcert)
            logger.info('Installing generated host key to %s', self.__test_hostkey)
            shutil.move(hostcertfiles.keyLocation, self.__test_hostkey)
        shutil.rmtree(tmpdir)

    def signusercert(self, args):
        ca = CertificateKeyPair(self.caKey, self.caCert)
        cg = CertificateGenerator('')
        timeidx = datetime.datetime.today().strftime('%m%d%H%M')
        username = 'Test Cert {}'.format(timeidx) if args.username is None else args.username
        usercertfiles = cg.generateClientCertificate(username, ca=ca,
                                                     validityperiod=args.validity, messagedigest=args.digest)
        if args.export_tar:
            workdir = os.getcwd()
            tarball = 'testcert-{0}.tar'.format(timeidx)
            tmpdir = tempfile.mkdtemp()
            os.chdir(tmpdir)
            export_dir = 'arc-test-certs'
            os.mkdir(export_dir)
            # move generated certs
            shutil.move(os.path.join(workdir, usercertfiles.certLocation), export_dir + '/usercert.pem')
            shutil.move(os.path.join(workdir, usercertfiles.keyLocation), export_dir + '/userkey.pem')
            # copy CA certificates
            shutil.copytree(self.x509_cert_dir, export_dir + '/certificates', symlinks=True)
            # remove private key
            os.unlink(export_dir + '/certificates/' + self.caName.replace(' ', '-') + '-key.pem')
            # create script to source
            script_content = 'basedir=$(dirname `readlink -f -- $_`)\n' \
                             'export X509_USER_CERT="$basedir/usercert.pem"\n' \
                             'export X509_USER_KEY="$basedir/userkey.pem"\n' \
                             'export X509_USER_PROXY="$basedir/userproxy.pem"\n' \
                             'export X509_CERT_DIR="$basedir/certificates"'
            with open(export_dir + '/usecerts.sh', 'w') as sf:
                sf.write(script_content)
            # make a tarball
            with tarfile.open(os.path.join(workdir, tarball), 'w') as tarf:
                tarf.add(export_dir)
            print('User certificate and key are exported to {0}.\n' \
                  'To use test cert with arc* tools on the other machine, copy the tarball and run following:\n' \
                  '  tar xf {0}\n' \
                  '  source {1}/usercerts.sh'.format(tarball, export_dir))
            # cleanup
            os.chdir(workdir)
            shutil.rmtree(tmpdir)
        else:
            print('User certificate and key are saved to {0} and {1} respectively.\n' \
                  'To use test cert with arc* tools export the following variables:\n' \
                  '  export X509_USER_CERT="{2}/{0}"\n' \
                  '  export X509_USER_KEY="{2}/{1}"'.format(
                        usercertfiles.certLocation,
                        usercertfiles.keyLocation,
                        os.getcwd()))

    def control(self, args):
        if args.action == 'init':
            self.createca(args)
        elif args.action == 'hostcert':
            self.signhostcert(args)
        elif args.action == 'usercert':
            self.signusercert(args)
        else:
            self.logger.critical('Unsupported ARC Test CA action %s', args.action)
            sys.exit(1)

    @staticmethod
    def register_parser(root_parser):
        testca_ctl = root_parser.add_parser('test-ca', help='ARC Test CA control')
        testca_ctl.set_defaults(handler_class=TestCAControl)

        testca_actions = testca_ctl.add_subparsers(title='Test CA Actions', dest='action',
                                                   metavar='ACTION', help='DESCRIPTION')

        testca_init = testca_actions.add_parser('init', help='Generate self-signed TestCA files')
        add_parser_digest_validity(testca_init)
        testca_init.add_argument('-f', '--force', action='store_true', help='Overwrite files if exists')

        testca_host = testca_actions.add_parser('hostcert', help='Generate and sign testing host certificate')
        add_parser_digest_validity(testca_host, 30)
        testca_host.add_argument('-n', '--hostname', action='store',
                                 help='Generate certificate for specified hostname instead of this host')
        testca_host.add_argument('-f', '--force', action='store_true', help='Overwrite files if exists')

        testca_user = testca_actions.add_parser('usercert', help='Generate and sign testing user certificate')
        add_parser_digest_validity(testca_user, 30)
        testca_user.add_argument('-n', '--username', action='store',
                                 help='Use specified username instead of automatically generated')
        testca_user.add_argument('-t', '--export-tar', action='store_true',
                                 help='Export tar archive to use from another host')
