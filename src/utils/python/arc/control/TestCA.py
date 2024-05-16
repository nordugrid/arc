from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
from .CertificateGenerator import CertificateGenerator, CertificateKeyPair
import subprocess
import socket
import sys
import stat
import tempfile
import shutil
import random
import tarfile
import pwd
from contextlib import closing


def add_parser_digest_validity(parser, defvalidity=90):
    parser.add_argument('-d', '--digest', help='Digest to use (default is %(default)s)', default='sha256',
                        choices=['md2', 'md4', 'md5', 'mdc2', 'sha1', 'sha224', 'sha256', 'sha384', 'sha512'])
    parser.add_argument('-v', '--validity', type=int, default=defvalidity,
                        help='Validity of certificate in days (default is %(default)s)')


class TestCAControl(ComponentControl):
    __test_hostcert = '/etc/grid-security/testCA-hostcert.pem'
    __test_hostkey = '/etc/grid-security/testCA-hostkey.pem'
    __test_authfile = '/etc/grid-security/testCA.allowed-subjects'

    __conf_d_access = '10-testCA-access.conf'
    __conf_d_hostcert = '00-testCA-hostcert.conf'

    def __arc_conf_access(self):
        """Template for arc.conf: allow access to testCA issued certs"""
        conf = [
            "#\n# Allow testCA issued certificates to submit jobs\n#",
            "\n[authgroup:testCA]",
            "file = {0}",
            "\n[mapping]",
            "map_to_user = testCA nobody:nobody",
            "\n[arex/ws/jobs]",
            "allowaccess = testCA"
        ]
        return '\n'.join(conf).format(self.__test_authfile)

    def __arc_conf_hostcert(self):
        """Template for arc.conf: hostcerts from testCA"""
        conf = [
            "#\n# Use host certificate signed by testCA\n#",
            "\n[common]",
            "x509_host_key = {0}",
            "x509_host_cert = {1}"
        ]
        return '\n'.join(conf).format(
            self.__test_hostkey,
            self.__test_hostcert
        )

    def __define_CA_ID(self, caid=None):
        """Internal function to define CA ID and file paths"""
        if caid is None:
            # CRC32 hostname-based hash used by default
            caid = crc32_id(self.hostname)
        self.caName = 'ARC TestCA {0}'.format(caid)
        self.caKey = os.path.join(self.x509_cert_dir, self.caName.replace(' ', '-') + '-key.pem')
        self.caCert = os.path.join(self.x509_cert_dir, self.caName.replace(' ', '-') + '.pem')

    def __define_ca_dir(self, dir):
        """Define alternative CA dir"""
        self.x509_cert_dir = dir
        self.__define_CA_ID()

    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.TestCA')
        self.x509_cert_dir = '/etc/grid-security/certificates'
        self.hostname = None

        # Use values from arc.conf if possible
        self.arcconfig = arcconfig
        if arcconfig is None:
            if arcctl_server_mode():
                self.logger.info('Failed to parse arc.conf, using default CA certificates directory')
            else:
                self.logger.debug('Working in config-less mode. Default paths will be used.')
        else:
            x509_cert_dir = arcconfig.get_value('x509_cert_dir', 'common')
            if x509_cert_dir:
                self.x509_cert_dir = x509_cert_dir
            self.hostname = arcconfig.get_value('hostname', 'common')
            self.logger.debug('Using hostname from arc.conf: %s', self.hostname)

        # if hostname is not defined via arc.conf
        if self.hostname is None:
            try:
                # try to get it from hostname -f
                hostname_f = subprocess.Popen(['hostname', '-f'], stdout=subprocess.PIPE)
                self.hostname = hostname_f.stdout.readline().decode().strip()
                self.logger.debug('Using hostname from \'hostname -f\': %s', self.hostname)
            except OSError:
                # fallback
                self.hostname = socket.gethostname()
                self.logger.warning('Cannot get hostname from \'hostname -f\'. '
                                    'Using %s that comes from name services.', self.hostname)

        # check hostname X509 constraint
        if len(str(self.hostname)) > 64:
            self.logger.error('Hostname %s is longer that 64 characters and does not fit to X509 subject limit.')
            sys.exit(1)

        # define CA name and paths
        self.__define_CA_ID()

    def createca(self, args):
        # CA certificates dir
        if not os.path.exists(self.x509_cert_dir):
            self.logger.debug('Making CA certificates directory at %s', self.x509_cert_dir)
            os.makedirs(self.x509_cert_dir, mode=0o755)
        # CA name from hostname
        cg = CertificateGenerator(self.x509_cert_dir)
        cg.generateCA(self.caName, validityperiod=args.validity, messagedigest=args.digest, force=args.force)
        # add arc.conf to authorize testCA users
        write_conf_d(self.__conf_d_access, self.__arc_conf_access())

    def cleanup_files(self):
        # CA certificates dir
        if not os.path.exists(self.x509_cert_dir):
            self.logger.debug('Making CA certificates directory at %s', self.x509_cert_dir)
            os.makedirs(self.x509_cert_dir, mode=0o755)
        # CA files cleanup
        cg = CertificateGenerator(self.x509_cert_dir)
        cg.cleanupCAfiles(self.caName)
        # hostcert/key, auth and conf.d files cleanup
        for f in (self.__test_hostcert,
                  self.__test_hostkey,
                  self.__test_authfile,
                  conf_d(self.__conf_d_access),
                  conf_d(self.__conf_d_hostcert)):
            if os.path.exists(f):
                self.logger.debug('Removing the file: %s', f)
                os.unlink(f)

    def signhostcert(self, args):
        ca = CertificateKeyPair(self.caKey, self.caCert)
        tmpdir = tempfile.mkdtemp()
        cg = CertificateGenerator(tmpdir)
        hostname = self.hostname if args.hostname is None else args.hostname
        hostcertfiles = cg.generateHostCertificate(hostname, ca=ca, validityperiod=args.validity,
                                                   messagedigest=args.digest)
        if args.export_tar:
            workdir = os.getcwd()
            tarball = 'hostcert-{0}.tar.gz'.format(hostname)
            exportdir = tempfile.mkdtemp()
            certdir = exportdir + '/certificates'
            os.mkdir(certdir)
            # move generated certs
            shutil.move(hostcertfiles.certLocation, exportdir + '/hostcert.pem')
            shutil.move(hostcertfiles.keyLocation, exportdir + '/hostkey.pem')
            # copy TestCA files
            cafiles = cg.getCAfiles(self.caName, self.x509_cert_dir)
            for cafile in cafiles['files']:
                if cafile.endswith('.srl') or cafile == self.caKey:
                    continue
                shutil.copy2(cafile, certdir)
            os.chdir(certdir)
            for cafile, linkto in cafiles['links']:
                os.symlink(linkto, cafile.replace(self.x509_cert_dir.rstrip('/') + '/', ''))
            os.chdir(exportdir)
            # create tarball
            with closing(tarfile.open(os.path.join(workdir, tarball), 'w:gz')) as tarf:
                tarf.add('.')
            # cleanup
            os.chdir(workdir)
            shutil.rmtree(exportdir)
        elif args.hostname is not None:
            workdir = os.getcwd()
            certfname = hostcertfiles.certLocation.split('/')[-1]
            keyfname = hostcertfiles.keyLocation.split('/')[-1]
            if not args.force:
                if os.path.exists(os.path.join(workdir, certfname)) or os.path.exists(os.path.join(workdir, keyfname)):
                    self.logger.error('Host certificate for %s already exists.', hostname)
                    shutil.rmtree(tmpdir)
                    sys.exit(1)
            shutil.move(hostcertfiles.certLocation, os.path.join(workdir, certfname))
            shutil.move(hostcertfiles.keyLocation, os.path.join(workdir, keyfname))
            os.chmod(os.path.join(workdir, keyfname), stat.S_IRUSR | stat.S_IWUSR)
            print('Host certificate and key are saved to {0} and {1} respectively.'.format(certfname, keyfname))
        else:
            if not args.force:
                if os.path.exists(self.__test_hostcert) or os.path.exists(self.__test_hostkey):
                    logger.error('Host certificate already exists.')
                    shutil.rmtree(tmpdir)
                    sys.exit(1)
            logger.info('Installing generated host certificate to %s', self.__test_hostcert)
            shutil.move(hostcertfiles.certLocation, self.__test_hostcert)
            logger.info('Installing generated host key to %s', self.__test_hostkey)
            shutil.move(hostcertfiles.keyLocation, self.__test_hostkey)
            conf_d = write_conf_d(self.__conf_d_hostcert, self.__arc_conf_hostcert())
            logger.info('Hostcert location configuration written to %s', conf_d)
        shutil.rmtree(tmpdir)

    @staticmethod
    def _remove_certs_and_exit(certfiles, exit_code=1):
        try:
            os.unlink(certfiles.certLocation)
            os.unlink(certfiles.keyLocation)
        except OSError:
            pass
        sys.exit(exit_code)

    def signusercert(self, args):
        ca = CertificateKeyPair(self.caKey, self.caCert)
        cg = CertificateGenerator('')
        randidx = random.randint(10000000, 99999999)
        username = 'Test User {0}'.format(randidx) if args.username is None else args.username
        usercertfiles = cg.generateClientCertificate(username, ca=ca,
                                                     validityperiod=args.validity, messagedigest=args.digest)
        if args.install_user is not None:
            try:
                pw = pwd.getpwnam(args.install_user)
            except KeyError:
                self.logger.error('Specified user %s does not exist.', args.install_user)
                self._remove_certs_and_exit(usercertfiles)
            # get homedir
            homedir = pw.pw_dir
            if not os.path.isdir(homedir):
                self.logger.error('Home directory %s does not exist for user %s', homedir, args.install_user)
                self._remove_certs_and_exit(usercertfiles)
            # .globus usercerts location
            usercertsdir = homedir + '/.globus'
            if os.path.exists(usercertsdir + '/usercert.pem') or os.path.exists(usercertsdir + '/userkey.pem'):
                if not args.force:
                    self.logger.error('User credentials already exist in %s. Use \'--force\' to overwrite.',
                                      usercertsdir)
                    self._remove_certs_and_exit(usercertfiles)
            if not os.path.isdir(usercertsdir):
                self.logger.debug('Creating %s directory to store user\'s credentials', usercertsdir)
                os.mkdir(usercertsdir)
                os.chmod(usercertsdir, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR
                         | stat.S_IRGRP | stat.S_IXGRP
                         | stat.S_IROTH | stat.S_IXOTH)
                os.chown(usercertsdir, pw.pw_uid, pw.pw_gid)
            # move cert/key
            shutil.move(usercertfiles.certLocation, usercertsdir + '/usercert.pem')
            shutil.move(usercertfiles.keyLocation, usercertsdir + '/userkey.pem')
            # chown cert/key
            self.logger.debug('Installing user certificate and key')
            os.chown(usercertsdir + '/usercert.pem', pw.pw_uid, pw.pw_gid)
            os.chown(usercertsdir + '/userkey.pem', pw.pw_uid, pw.pw_gid)
            print('User certificate and key are installed to default {0} location for user {1}.'
                  .format(usercertsdir, args.install_user))
        elif args.export_tar:
            workdir = os.getcwd()
            tarball = 'usercert-{0}.tar.gz'.format(username.replace(' ', '-'))
            tmpdir = tempfile.mkdtemp()
            os.chdir(tmpdir)
            export_dir = 'arc-testca-usercert'
            os.mkdir(export_dir)
            # move generated certs
            shutil.move(os.path.join(workdir, usercertfiles.certLocation), export_dir + '/usercert.pem')
            shutil.move(os.path.join(workdir, usercertfiles.keyLocation), export_dir + '/userkey.pem')
            # copy CA certificates
            shutil.copytree(self.x509_cert_dir, export_dir + '/certificates', symlinks=True)
            # remove private key
            os.unlink(export_dir + '/certificates/' + self.caName.replace(' ', '-') + '-key.pem')
            # create script to source
            script_content = 'basedir=$(dirname `readlink -f -- ${BASH_SOURCE:-$_}`)\n' \
                             'export X509_USER_CERT="$basedir/usercert.pem"\n' \
                             'export X509_USER_KEY="$basedir/userkey.pem"\n' \
                             'export X509_USER_PROXY="$basedir/userproxy.pem"\n' \
                             'export X509_CERT_DIR="$basedir/certificates"'
            with open(export_dir + '/setenv.sh', 'w') as sf:
                sf.write(script_content)
            # make a tarball
            with closing(tarfile.open(os.path.join(workdir, tarball), 'w:gz')) as tarf:
                tarf.add(export_dir)
            print('User certificate and key are exported to {0}.\n'
                  'To use it with arc* tools on the other machine, copy the tarball and run the following commands:\n'
                  '  tar xzf {0}\n'
                  '  source {1}/setenv.sh'.format(tarball, export_dir))
            # cleanup
            os.chdir(workdir)
            shutil.rmtree(tmpdir)
        else:
            print('User certificate and key are saved to {0} and {1} respectively.\n'
                  'To use test cert with arc* tools export the following variables:\n'
                  '  export X509_USER_CERT="{2}/{0}"\n'
                  '  export X509_USER_KEY="{2}/{1}"'.format(
                        usercertfiles.certLocation,
                        usercertfiles.keyLocation,
                        os.getcwd()))
        # add subject to allowed list
        if arcctl_server_mode():
            if not args.no_auth:
                try:
                    self.logger.info('Adding certificate subject name (%s) to allowed list at %s',
                                     usercertfiles.dn, self.__test_authfile)
                    with open(self.__test_authfile, 'a') as a_file:
                        a_file.write('"{0}"\n'.format(usercertfiles.dn))
                except IOError as err:
                    self.logger.error('Failed to modify %s. Error: %s', self.__test_authfile, str(err))
                    sys.exit(1)

    def control(self, args):
        # define CA dir if provided
        if args.ca_dir is not None:
            self.__define_ca_dir(args.ca_dir)
        # no need to go further if it CA dir is not writable
        ensure_path_writable(self.x509_cert_dir)
        # define CA ID if provided
        if args.ca_id is not None:
            self.__define_CA_ID(args.ca_id)
        # parse actions
        if args.action == 'init':
            self.createca(args)
        elif args.action == 'cleanup':
            self.cleanup_files()
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

        testca_ctl.add_argument('--ca-id', action='store',
                                help='Define CA ID to work with (default is to use hostname-based hash)')
        testca_ctl.add_argument('--ca-dir', action='store',
                                help='Redefine path to CA files directory')

        testca_actions = testca_ctl.add_subparsers(title='Test CA Actions', dest='action',
                                                   metavar='ACTION', help='DESCRIPTION')
        testca_actions.required = True

        testca_init = testca_actions.add_parser('init', help='Generate self-signed TestCA files')
        add_parser_digest_validity(testca_init)
        testca_init.add_argument('-f', '--force', action='store_true', help='Overwrite files if exist')

        testca_cleanup = testca_actions.add_parser('cleanup', help='Cleanup TestCA files')

        testca_host = testca_actions.add_parser('hostcert', help='Generate and sign testing host certificate')
        add_parser_digest_validity(testca_host, 30)
        testca_host.add_argument('-n', '--hostname', action='store',
                                 help='Generate certificate for specified hostname instead of this host')
        testca_host.add_argument('-f', '--force', action='store_true', help='Overwrite files if exist')
        testca_host.add_argument('-t', '--export-tar', action='store_true',
                                 help='Export tar archive to use from another host')

        testca_user = testca_actions.add_parser('usercert', help='Generate and sign testing user certificate')
        add_parser_digest_validity(testca_user, 30)
        testca_user.add_argument('-n', '--username', action='store',
                                 help='Use specified username instead of automatically generated')
        testca_user.add_argument('-i', '--install-user', action='store',
                                 help='Install certificates to $HOME/.globus for specified user instead of workdir')
        testca_user.add_argument('-t', '--export-tar', action='store_true',
                                 help='Export tar archive to use from another host')
        testca_user.add_argument('-f', '--force', action='store_true', help='Overwrite files if exist')
        if arcctl_server_mode():
            testca_user.add_argument('--no-auth', action='store_true', help='Do not add user subject to allowed list')
