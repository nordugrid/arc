import subprocess
import tempfile
import os
import argparse
import sys
import stat
import logging


def popen(cmd):
    logger.info('Running the following command: %s', ' '.join(cmd))
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    logger.debug('Command stdout: %s', stdout)
    logger.debug('Command stderr: %s', stderr)
    return {'cmd': cmd, 'returncode': proc.returncode, 'stdout': stdout, 'stderr': stderr}


class CertificateKeyPair(object):
    def __init__(self, keyLocation, certLocation, dn=""):
        super(CertificateKeyPair, self).__init__()
        self.keyLocation = keyLocation
        self.certLocation = certLocation
        self.signingPolicyLocation = ""
        self.dn = dn
        self.subject_hash = ""
        self.subject_hash_old = ""


class CertificateGenerator(object):
    supportedMessageDigests = ["md2", "md4", "md5", "mdc2", "sha1", "sha224", "sha256", "sha384", "sha512"]

    def __init__(self, work_dir=''):
        super(CertificateGenerator, self).__init__()
        self._ca = None
        self.work_dir = work_dir

    @staticmethod
    def checkMessageDigest(messagedigest):
        if messagedigest not in CertificateGenerator.supportedMessageDigests:
            raise Exception("The message digest \"%s\" is not supported" % messagedigest)

    def generateCA(self, name="Test CA", validityperiod=30, messagedigest="sha1", use_for_signing=True, force=False):
        if not isinstance(validityperiod, (int, long)):
            raise Exception("The 'validityperiod' argument must be an integer")

        CertificateGenerator.checkMessageDigest(messagedigest)

        keyLocation = os.path.join(self.work_dir, name.replace(" ", "-") + "-key.pem")
        certLocation = os.path.join(self.work_dir, name.replace(" ", "-") + ".pem")
        if os.path.isfile(keyLocation):
            if force:
                logger.info("Key file '%s' already exist. Removing. ", keyLocation)
                os.unlink(keyLocation)
            else:
                logger.error("Error generating CA certificate and key: file '%s' already exist", keyLocation)
                sys.exit(1)
        if os.path.isfile(certLocation):
            if force:
                logger.info("Certificate file '%s' already exist. Removing. ", certLocation)
                # remove the links
                for fname in os.listdir(self.work_dir):
                    if fname.endswith('.0'):
                        fpath = os.path.join(self.work_dir, fname)
                        if os.readlink(fpath) == name.replace(" ", "-") + ".pem":
                            os.unlink(fpath)
                # clean the cert itself
                os.unlink(certLocation)
            else:
                logger.error("Error generating CA certificate and key: file '%s' already exist", certLocation)
                sys.exit(1)

        subject = "/DC=org/DC=nordugrid/DC=ARC/O=TestCA/CN=" + name
        logger.info('Generating Test CA %s', subject)
        if popen(["openssl", "genrsa", "-out", keyLocation, "2048"])["returncode"] != 0:
            raise Exception()
        if popen(["openssl", "req", "-x509", "-new", "-" + messagedigest, "-subj", subject, "-key",
                  keyLocation, "-out", certLocation, "-days", str(validityperiod)])["returncode"] != 0:
            raise Exception()

        ca = CertificateKeyPair(keyLocation, certLocation, subject)
        if use_for_signing:
            self._ca = ca

        os.chmod(keyLocation, stat.S_IRUSR)
        os.chmod(certLocation, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

        # Order of the -subject_hash and -subject_hash_old flags matters.
        p_handle = popen(["openssl", "x509", "-subject_hash", "-subject_hash_old", "-noout", "-in", certLocation])
        if p_handle["returncode"] == 0:
            ca.subject_hash, ca.subject_hash_old = p_handle["stdout"].splitlines()
            # Use relative location. Assume hash link does not already exist (.0).
            certFilename = name.replace(" ", "-") + ".pem"
            os.chdir(self.work_dir)
            logger.info('Linking %s to %s.0', certFilename, ca.subject_hash)
            os.symlink(certFilename, ca.subject_hash + ".0")
            logger.info('Linking %s to %s.0', certFilename, ca.subject_hash_old)
            os.symlink(certFilename, ca.subject_hash_old + ".0")
        else:
            logger.error('Failed to calculate certificate hash values. Cleaning up generated files.')
            os.unlink(keyLocation)
            os.unlink(certLocation)
            sys.exit(1)

        logger.info('Writing signing_policy file for CA')
        ca.signingPolicyLocation = os.path.join(self.work_dir, name.replace(" ", "-") + ".signing_policy")
        signing_policy = '''# EACL ARC Test Utility
access_id_CA  X509   '{subject}'
pos_rights    globus CA:sign
cond_subjects globus '"{cond_subject}/*"'
'''.format(subject=subject, cond_subject=subject[:subject.rfind('/')])
        with open(ca.signingPolicyLocation, "w") as f_signing:
            f_signing.write(signing_policy)
        return ca

    def generateHostCertificate(self, hostname, prefix="host", ca=None,
                                validityperiod=30, messagedigest="sha1", force=False):
        if ca is None and self._ca is None:
            raise Exception("No CA provided")
        if not isinstance(validityperiod, (int, long)):
            raise Exception("The 'validityperiod' argument must be an integer")

        if ca is None:
            ca = self._ca

        CertificateGenerator.checkMessageDigest(messagedigest)

        prefix += "-" + hostname.replace(" ", "-")
        keyLocation = os.path.join(self.work_dir, prefix + "-key.pem")
        certReqFile, certReqLocation = tempfile.mkstemp('-cert-req.pem', prefix)
        os.close(certReqFile)
        certLocation = os.path.join(self.work_dir, prefix + "-cert.pem")

        if os.path.isfile(keyLocation):
            if force:
                logger.info("Key file '%s' already exist. Removing. ", keyLocation)
                os.unlink(keyLocation)
            else:
                logger.error("Error generating host certificate and key: file '%s' already exist", keyLocation)
                sys.exit(1)
        if os.path.isfile(certLocation):
            if force:
                logger.info("Certificate file '%s' already exist. Removing. ", certLocation)
                os.unlink(certLocation)
            else:
                logger.error("Error generating host certificate and key: file '%s' already exist", certLocation)
                sys.exit(1)

        logger.info('Generating host certificate signing request.')
        subject = "/DC=org/DC=nordugrid/DC=ARC/O=TestCA/CN=host\/" + hostname
        if popen(["openssl", "genrsa", "-out", keyLocation, "2048"])["returncode"] != 0:
            raise Exception()
        if popen(["openssl", "req", "-new", "-" + messagedigest, "-subj", subject,
                  "-key", keyLocation, "-out", certReqLocation])["returncode"] != 0:
            raise Exception()

        config_descriptor, config_name = tempfile.mkstemp(prefix="x509v3_config-")
        config = os.fdopen(config_descriptor, "w")
        config.write("basicConstraints=CA:FALSE\n")
        config.write("keyUsage=digitalSignature, nonRepudiation, keyEncipherment\n")
        config.write("subjectAltName=DNS:" + hostname + "\n")
        config.close()

        logger.info('Signing host certificate with Test CA.')
        if popen(["openssl", "x509", "-req", "-" + messagedigest, "-in", certReqLocation, "-CA", ca.certLocation,
                  "-CAkey", ca.keyLocation, "-CAcreateserial", "-extfile", config_name, "-out", certLocation,
                  "-days", str(validityperiod)])["returncode"] != 0:
            raise Exception()

        os.remove(certReqLocation)
        os.remove(config_name)

        os.chmod(keyLocation, stat.S_IRUSR)
        os.chmod(certLocation, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

        return CertificateKeyPair(keyLocation, certLocation, subject)

    def generateClientCertificate(self, name, prefix="client", ca=None, validityperiod=30, messagedigest="sha1"):
        if ca is None and self._ca is None:
            raise Exception("No CA provided")
        if not isinstance(validityperiod, (int, long)):
            raise Exception("The 'validityperiod' argument must be an integer")

        if ca is None:
            ca = self._ca

        CertificateGenerator.checkMessageDigest(messagedigest)

        prefix += "-" + name.replace(" ", "-")
        keyLocation = os.path.join(self.work_dir, prefix + "-key.pem")
        certReqFile, certReqLocation = tempfile.mkstemp('-cert-req.pem', prefix)
        os.close(certReqFile)
        certLocation = os.path.join(self.work_dir, prefix + "-cert.pem")

        if os.path.isfile(keyLocation):
            logger.error("Error generating client certificate and key: file '%s' already exist", keyLocation)
            sys.exit(1)
        if os.path.isfile(certLocation):
            logger.error("Error generating client certificate and key: file '%s' already exist", certLocation)
            sys.exit(1)

        logger.info('Generating client certificate signing request.')
        subject = "/DC=org/DC=nordugrid/DC=ARC/O=TestCA/CN=" + name
        if popen(["openssl", "genrsa", "-out", keyLocation, "2048"])["returncode"] != 0:
            raise Exception()
        if popen(["openssl", "req", "-new", "-" + messagedigest, "-subj", subject,
                  "-key", keyLocation, "-out", certReqLocation])["returncode"] != 0:
            raise Exception()

        config_descriptor, config_name = tempfile.mkstemp(prefix="x509v3_config-")
        config = os.fdopen(config_descriptor, "w")
        config.write("basicConstraints=CA:FALSE\n")
        config.write("keyUsage=digitalSignature, nonRepudiation, keyEncipherment\n")
        config.close()

        logger.info('Signing client certificate with Test CA.')
        if popen(["openssl", "x509", "-req", "-" + messagedigest, "-in", certReqLocation, "-CA",
                  ca.certLocation, "-CAkey", ca.keyLocation, "-CAcreateserial", "-extfile", config_name,
                  "-out", certLocation, "-days", str(validityperiod)])["returncode"] != 0:
            raise Exception()

        os.remove(certReqLocation)
        os.remove(config_name)

        os.chmod(keyLocation, stat.S_IRUSR)
        os.chmod(certLocation, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

        return CertificateKeyPair(keyLocation, certLocation, subject)


def createParser():
    parser = argparse.ArgumentParser(description='Script for generating certificates')
    parser.add_argument('--CA', help='Generate CA certificate with supplied name')
    parser.add_argument('--host', help='Generate host certificate with supplied name')
    parser.add_argument('--client', help='Generate client certificate with supplied name')
    parser.add_argument('--CA-key-path', help='Path of CA key')
    parser.add_argument('--CA-cert-path', help='Path of CA certificate')
    parser.add_argument('--validity', type=int, default=30,
                        help='Number of days the certificates will be valid (default %(default)s)')
    parser.add_argument('--digest', default="sha1", help='The hash function to use for certificate signing')
    parser.add_argument('--list-digest', action='store_const', const=True, help='List supported hash functions')
    return parser


if __name__ == "__main__":
    logger = logging.getLogger('CertificateGenerator')
    logger.setLevel(logging.DEBUG)
    logger.addHandler(logging.StreamHandler())

    parser = createParser()
    args = parser.parse_args()

    if args.list_digest:
        print "Supported hash functions are: %s" % ", ".join(CertificateGenerator.supportedMessageDigests)
        sys.exit(0)

    if args.CA is None and args.host is None and args.client is None:
        parser.print_help()
        print "Error: At least one of the options '--CA', '--host', '--client' must be specified."
        sys.exit(-1)

    if args.CA and (args.CA_key_path or args.CA_cert_path):
        parser.print_help()
        print "Error: '--CA' may not be specified with either '--CA-key-path' or '--CA-cert-path'."
        sys.exit(-1)

    if args.CA_key_path and not args.CA_cert_path or not args.CA_key_path and args.CA_cert_path:
        parser.print_help()
        print "Error: Both '--CA-key-path' and '--CA-cert-path' must be specified."
        sys.exit(-1)

    if (args.host or args.client) and not (args.CA or args.CA_key_path):
        parser.print_help()
        print "Error: When generating host or client certificates. " \
              "Either '--CA' or path to existing CA certificates must be specified."
        sys.exit(-1)

    try:
        CertificateGenerator.checkMessageDigest(args.digest)
    except Exception as e:
        print e
        print "Supported hash functions are: %s" % ", ".join(CertificateGenerator.supportedMessageDigests)
        sys.exit(-1)

    cc = CertificateGenerator()

    ca = None
    if args.CA:
        print "Generating CA certificate and key."
        ca = cc.generateCA(args.CA, validityperiod=args.validity, messagedigest=args.digest)
    else:
        print "Using specified CA certificate and key."
        ca = CertificateKeyPair(args.CA_key_path, args.CA_cert_path)

    if args.host:
        print "Generating host certificate."
        cc.generateHostCertificate(args.host, ca=ca, validityperiod=args.validity, messagedigest=args.digest)

    if args.client:
        print "Generating client certificate."
        cc.generateClientCertificate(args.client, ca=ca, validityperiod=args.validity, messagedigest=args.digest)

    sys.exit(0)
else:
    logger = logging.getLogger('ARCCTL.CertificateGenerator')
