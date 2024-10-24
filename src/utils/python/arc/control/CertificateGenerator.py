from __future__ import print_function

import tempfile
import os
import sys
import stat
import logging

from .ControlCommon import run_subprocess

class CertificateKeyPair(object):
    def __init__(self, keyLocation, certLocation, dn=""):
        super(CertificateKeyPair, self).__init__()
        self.logger = logging.getLogger('ARCCTL.CertificateKeyPair')

        self.keyLocation = keyLocation
        self.certLocation = certLocation

        if dn is None:
            self.dn = run_subprocess("openssl", "x509", "-subject", "-noout",
                                     "-nameopt", "compat",
                                     "-in", self.certLocation).split('=', 1)[1].strip()
            self.logger.debug('Certificate DN: %s', self.dn)
        else:
            self.dn = dn

        self.subject_hash = ""
        self.subject_hash_old = ""

        self.certDir, self.certFilename = os.path.split(certLocation)
        self.certBasename = self.certFilename.rsplit('.', 1)[0]
        self.signingPolicyFile = self.certBasename + ".signing_policy"

    def setFilePermissions(self):
        if self.keyLocation is not None:
            os.chmod(self.keyLocation, stat.S_IRUSR)
        os.chmod(self.certLocation, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)


    def setCertificateSubjectHash(self, force=False):
        if self.subject_hash and not force:
            self.logger.debug('Certificate subject is already set for %s', self.certLocation)
            return
        self.subject_hash, self.subject_hash_old = run_subprocess(
            "openssl", "x509", "-subject_hash", "-subject_hash_old",
            "-noout", "-in", self.certLocation).splitlines()

    def makeHashLinks(self):
        self.setCertificateSubjectHash()
        # Use relative location. Assume hash link does not already exist (.0).
        os.chdir(self.certDir)
        self.logger.info('Linking %s to %s.0', self.certFilename, self.subject_hash)
        os.symlink(self.certFilename, self.subject_hash + ".0")
        self.logger.info('Linking %s to %s.0', self.certFilename, self.subject_hash_old)
        os.symlink(self.certFilename, self.subject_hash_old + ".0")

    def writeCASigningPolicy(self):
        # Signing policy is critical for Globus
        self.logger.info('Writing signing_policy file for CA')
        signing_policy = '''# EACL for {dn}
access_id_CA  X509   '{dn}'
pos_rights    globus CA:sign
cond_subjects globus '"{dn_base}/*"'
'''.format(dn=self.dn, dn_base=self.dn[:self.dn.rfind('/')])
        with open(os.path.join(self.certDir,self.signingPolicyFile), "w") as f_signing:
            f_signing.write(signing_policy)
        # hash links
        self.setCertificateSubjectHash()
        os.chdir(self.certDir)
        self.logger.info('Linking %s to %s.signing_policy', self.signingPolicyFile, self.subject_hash)
        os.symlink(self.signingPolicyFile, self.subject_hash + ".signing_policy")
        self.logger.info('Linking %s to %s.signing_policy', self.signingPolicyFile, self.subject_hash_old)
        os.symlink(self.signingPolicyFile, self.subject_hash_old + ".signing_policy")

class CertificateGenerator(object):
    supportedMessageDigests = ["sha224", "sha256", "sha384", "sha512"]

    def __init__(self, work_dir=''):
        super(CertificateGenerator, self).__init__()
        self.logger = logging.getLogger('ARCCTL.CertificateGenerator')
        self._ca = None
        self.work_dir = work_dir

    def checkMessageDigest(self, messagedigest):
        if messagedigest not in self.supportedMessageDigests:
            self.logger.error("The message digest \"%s\" is not supported", messagedigest)
            sys.exit(1)

    def getCAfiles(self, name="Test CA", work_dir=None):
        cafiles = {'links': [], 'files': []}
        if work_dir is None:
            work_dir = self.work_dir
        namelen = len(name)
        dashname = name.replace(" ", "-")
        # get the symlinks pointing to CA files
        for fname in os.listdir(work_dir):
            fpath = os.path.join(work_dir, fname)
            if os.path.islink(fpath):
                linkto = os.readlink(fpath).replace(work_dir.rstrip('/') + '/', '')
                if linkto[0:namelen] == dashname:
                    cafiles['links'].append((fpath, linkto))
        # ca files itself
        for fname in os.listdir(work_dir):
            fpath = os.path.join(work_dir, fname)
            if os.path.isfile(fpath):
                if fname[0:namelen] == dashname:
                    cafiles['files'].append(fpath)
        return cafiles

    def cleanupCAfiles(self, name="Test CA"):
        cafiles = self.getCAfiles(name)
        for (fpath, linkto) in cafiles['links']:
            self.logger.info('Removing the CA link: %s -> %s', fpath, linkto)
            os.unlink(fpath)
        for fpath in cafiles['files']:
            self.logger.info('Removing the CA file: %s', fpath)
            os.unlink(fpath)

    def generateCA(self, name="Test CA", validityperiod=30, messagedigest="sha384", use_for_signing=True, force=False):
        if not isinstance(validityperiod, int):
            self.logger.error("The 'validityperiod' argument must be an integer")
            sys.exit(1)

        self.checkMessageDigest(messagedigest)

        keyLocation = os.path.join(self.work_dir, name.replace(" ", "-") + "-key.pem")
        certLocation = os.path.join(self.work_dir, name.replace(" ", "-") + ".pem")
        if os.path.isfile(keyLocation):
            if force:
                self.logger.info("Key file '%s' already exist. Cleaning up previous Test-CA files.", keyLocation)
                self.cleanupCAfiles(name)
            else:
                self.logger.error("Error generating CA certificate and key: file '%s' is already exist", keyLocation)
                sys.exit(1)
        if os.path.isfile(certLocation):
            if force:
                self.logger.info("Certificate file '%s' already exist. Cleaning up previous Test-CA files.", certLocation)
                self.cleanupCAfiles(name)
            else:
                self.logger.error("Error generating CA certificate and key: file '%s' is already exist", certLocation)
                sys.exit(1)

        subject = "/DC=org/DC=nordugrid/DC=ARC/O=TestCA/CN=" + name
        self.logger.info('Generating Test CA RSA Key for %s', subject)
        run_subprocess("openssl", "genrsa", "-out", keyLocation, "4096")

        run_subprocess("openssl", "req", "-x509", "-new", "-" + messagedigest, "-subj", subject, "-key",
                  keyLocation, "-out", certLocation, "-days", str(validityperiod))

        ca = CertificateKeyPair(keyLocation, certLocation, subject)

        if use_for_signing:
            self._ca = ca

        try:
            ca.setFilePermissions()
            ca.makeHashLinks()
            ca.writeCASigningPolicy()
        except OSError as e:
            self.logger.error('Failed to generate Test CA: %s.', str(e))
            os.unlink(keyLocation)
            os.unlink(certLocation)
            sys.exit(1)

        return ca

    def __check_generate_args(self, ca, validityperiod, messagedigest):
        """common validations for certificate generation"""
        if ca is None and self._ca is None:
            self.logger.error("No CA provided")
            sys.exit(1)
        if not isinstance(validityperiod, int):
            self.logger.error("The 'validityperiod' argument must be an integer")
            sys.exit(1)

        if ca is None:
            ca = self._ca

        try:
            with open(ca.keyLocation, 'r') as ca_key:
                pass
        except IOError as e:
            self.logger.error("Failed to access Test CA key. Error(%s): %s", e.errno, e.strerror)
            sys.exit(1)
        
        self.checkMessageDigest(messagedigest)

        return ca

    def generateHostCertificate(self, hostname, prefix="host", ca=None,
                                validityperiod=30, messagedigest="sha384", force=False):
        ca = self.__check_generate_args(ca, validityperiod, messagedigest)

        prefix += "-" + hostname.replace(" ", "-")
        keyLocation = os.path.join(self.work_dir, prefix + "-key.pem")
        certReqFile, certReqLocation = tempfile.mkstemp('-cert-req.pem', prefix)
        os.close(certReqFile)
        certLocation = os.path.join(self.work_dir, prefix + "-cert.pem")

        if os.path.isfile(keyLocation):
            if force:
                self.logger.info("Key file '%s' already exist. Removing. ", keyLocation)
                os.unlink(keyLocation)
            else:
                self.logger.error("Error generating host certificate and key: file '%s' already exist", keyLocation)
                sys.exit(1)
        if os.path.isfile(certLocation):
            if force:
                self.logger.info("Certificate file '%s' already exist. Removing. ", certLocation)
                os.unlink(certLocation)
            else:
                self.logger.error("Error generating host certificate and key: file '%s' already exist", certLocation)
                sys.exit(1)

        self.logger.info('Generating host certificate signing request.')
        subject = "/DC=org/DC=nordugrid/DC=ARC/O=TestCA/CN=host\\/" + hostname

        run_subprocess("openssl", "genrsa", "-out", keyLocation, "2048")

        run_subprocess("openssl", "req", "-new", "-" + messagedigest, "-subj", subject,
                  "-key", keyLocation, "-out", certReqLocation)

        config_descriptor, config_name = tempfile.mkstemp(prefix="x509v3_config-")
        config = os.fdopen(config_descriptor, "w")
        config.write("basicConstraints=CA:FALSE\n")
        config.write("keyUsage=digitalSignature, nonRepudiation, keyEncipherment\n")
        config.write("subjectAltName=DNS:" + hostname + "\n")
        config.close()

        self.logger.info('Signing host certificate with Test CA.')

        run_subprocess("openssl", "x509", "-req", "-" + messagedigest, "-in", certReqLocation, "-CA", ca.certLocation,
                  "-CAkey", ca.keyLocation, "-CAcreateserial", "-extfile", config_name, "-out", certLocation,
                  "-days", str(validityperiod))

        os.remove(certReqLocation)
        os.remove(config_name)

        keypair = CertificateKeyPair(keyLocation, certLocation, subject)
        keypair.setFilePermissions()

        return keypair

    def generateClientCertificate(self, name, prefix="client", ca=None, validityperiod=30, messagedigest="sha384"):
        ca = self.__check_generate_args(ca, validityperiod, messagedigest)

        prefix += "-" + name.replace(" ", "-")
        keyLocation = os.path.join(self.work_dir, prefix + "-key.pem")
        certReqFile, certReqLocation = tempfile.mkstemp('-cert-req.pem', prefix)
        os.close(certReqFile)
        certLocation = os.path.join(self.work_dir, prefix + "-cert.pem")

        if os.path.isfile(keyLocation):
            self.logger.error("Error generating client certificate and key: file '%s' already exist", keyLocation)
            os.remove(certReqLocation)
            sys.exit(1)
        if os.path.isfile(certLocation):
            self.logger.error("Error generating client certificate and key: file '%s' already exist", certLocation)
            os.remove(certReqLocation)
            sys.exit(1)

        self.logger.info('Generating client certificate signing request.')
        subject = "/DC=org/DC=nordugrid/DC=ARC/O=TestCA/CN=" + name

        run_subprocess("openssl", "genrsa", "-out", keyLocation, "2048")
        run_subprocess("openssl", "req", "-new", "-" + messagedigest, "-subj", subject,
                  "-key", keyLocation, "-out", certReqLocation)

        config_descriptor, config_name = tempfile.mkstemp(prefix="x509v3_config-")
        config = os.fdopen(config_descriptor, "w")
        config.write("basicConstraints=CA:FALSE\n")
        config.write("keyUsage=digitalSignature, nonRepudiation, keyEncipherment\n")
        config.close()

        self.logger.info('Signing client certificate with Test CA.')

        run_subprocess("openssl", "x509", "-req", "-" + messagedigest, "-in", certReqLocation, "-CA",
                  ca.certLocation, "-CAkey", ca.keyLocation, "-CAcreateserial", "-extfile", config_name,
                  "-out", certLocation, "-days", str(validityperiod))

        os.remove(certReqLocation)
        os.remove(config_name)

        keypair = CertificateKeyPair(keyLocation, certLocation, subject)
        keypair.setFilePermissions()

        return keypair

