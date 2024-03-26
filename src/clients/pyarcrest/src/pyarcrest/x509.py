import os
import re
import time
from datetime import datetime, timedelta

from cryptography import x509
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa

from pyarcrest.errors import X509Error

PROXYPATH = f"/tmp/x509up_u{os.getuid()}"


def isOldProxy(cert):
    """
    Return True if the given proxy Certificate object is in old format.

    The value of the last CN of an old format proxy is either "proxy" or
    "limited proxy".
    """
    lastCN = cert.subject.get_attributes_for_oid(x509.oid.NameOID.COMMON_NAME)[-1]
    return lastCN.value in ("proxy", "limited proxy")


def validKeyUsage(cert):
    """
    Return True if the given proxy Certificate object's key usage is valid.

    Key usage is considered valid if digital signature bit is set in extension
    or if there is no key usage extension.
    """
    try:
        keyUsage = cert.extensions.get_extension_for_oid(x509.oid.ExtensionOID.KEY_USAGE)
        return bool(keyUsage.value.digital_signature)
    except x509.ExtensionNotFound:
        return True


def checkRFCProxy(proxy):
    """Return True if the given Certificate object is a valid X.509 RFC 3820 proxy."""
    for ext in proxy.extensions:
        if ext.oid.dotted_string == "1.3.6.1.5.5.7.1.14":
            return True
    return False


def createProxyCSR(issuer, key):
    """
    Create proxy certificate signing request.

    Args:
        issuer:
            A proxy Certificate object of the proxied entity.
        key:
            A key object of the proxying entity.
            The type of object is one of RSAPrivateKey, DSAPrivateKey,
            EllipticCurvePrivateKey, Ed25519PrivateKey or Ed448PrivateKey.

    Returns:
        A CertificateSigningRequest object.
    """
    if isOldProxy(issuer):
        raise X509Error("Proxy format not supported")
    if not validKeyUsage(issuer):
        raise X509Error("Proxy uses invalid keyUsage extension")

    builder = x509.CertificateSigningRequestBuilder()

    # copy subject to CSR
    subject = list(issuer.subject)
    builder = builder.subject_name(x509.Name(subject))

    # add proxyCertInfo extension
    oid = x509.ObjectIdentifier("1.3.6.1.5.5.7.1.14")
    value = b"0\x0c0\n\x06\x08+\x06\x01\x05\x05\x07\x15\x01"
    extension = x509.extensions.UnrecognizedExtension(oid, value)
    builder = builder.add_extension(extension, critical=True)

    # sign the proxy CSR with the key
    return builder.sign(
        private_key=key,
        algorithm=hashes.SHA256(),
        backend=default_backend(),
    )


def signProxyCSR(csr, proxypath=PROXYPATH, lifetime=None):
    """
    Sign proxy CSR.

    Args:
        csr:
            A CertificateSigningRequest object.
        proxypath:
            A string of the path of the proxy file.
        lifetime:
            If None, the cert validity will be the same as that of the signing
            cert. Otherwise, it is used as a number of hours from now which the
            cert will be valid for.

    Returns:
        A proxy Certificate object.
    """
    now = datetime.utcnow()
    if not csr.is_signature_valid:
        raise X509Error("Invalid request signature")

    with open(proxypath, "rb") as f:
        proxy_pem = f.read()

    proxy = x509.load_pem_x509_certificate(proxy_pem, default_backend())
    if not checkRFCProxy(proxy):
        raise X509Error("Invalid RFC proxy")

    key = serialization.load_pem_private_key(proxy_pem, password=None, backend=default_backend())
    keyID = x509.SubjectKeyIdentifier.from_public_key(key.public_key())

    # add a CN with serial number to subject
    subject = list(proxy.subject)
    subject.append(x509.NameAttribute(x509.oid.NameOID.COMMON_NAME, str(int(time.time()))))

    cert_builder = x509.CertificateBuilder() \
        .issuer_name(proxy.subject) \
        .not_valid_before(now) \
        .serial_number(proxy.serial_number) \
        .public_key(csr.public_key()) \
        .subject_name(x509.Name(subject)) \
        .add_extension(x509.BasicConstraints(ca=False, path_length=None),
                       critical=True) \
        .add_extension(x509.KeyUsage(digital_signature=True,
                                     content_commitment=False,
                                     key_encipherment=False,
                                     data_encipherment=False,
                                     key_agreement=True,
                                     key_cert_sign=False,
                                     crl_sign=False,
                                     encipher_only=False,
                                     decipher_only=False),
                       critical=True) \
        .add_extension(x509.AuthorityKeyIdentifier(
            key_identifier=keyID.digest,
            authority_cert_issuer=[x509.DirectoryName(proxy.issuer)],
            authority_cert_serial_number=proxy.serial_number
            ),
                       critical=False) \
        .add_extension(x509.extensions.UnrecognizedExtension(
            x509.ObjectIdentifier("1.3.6.1.5.5.7.1.14"),
            b"0\x0c0\n\x06\x08+\x06\x01\x05\x05\x07\x15\x01"),
                       critical=True)

    if not lifetime:
        cert_builder = cert_builder.not_valid_after(proxy.not_valid_after)
    else:
        cert_builder = cert_builder.not_valid_after(now + timedelta(hours=lifetime))
    return cert_builder.sign(
        private_key=key,
        algorithm=proxy.signature_hash_algorithm,
        backend=default_backend()
    )


def parseProxyPEM(pem):
    """Return cert, key and chain PEMs from the given proxy PEM."""
    sections = re.findall(
        "-----BEGIN.*?-----.*?-----END.*?-----",
        pem,
        flags=re.DOTALL
    )

    try:
        certPEM = sections[0]
        keyPEM = sections[1]
        chainPEMs = sections[2:]
    except IndexError:
        raise X509Error("Invalid PEM")
    else:
        return f"{certPEM}\n", f"{keyPEM}\n", "\n".join(chainPEMs) + "\n"


def generateKey(size=2048):
    return rsa.generate_private_key(
        public_exponent=65537,  # the docs say that this value should be used
        key_size=size,
        backend=default_backend()
    )


def certToPEM(cert):
    return cert.public_bytes(serialization.Encoding.PEM).decode()


def pemToCert(pem):
    return x509.load_pem_x509_certificate(pem.encode(), default_backend())


def csrToPEM(csr):
    return certToPEM(csr)


def pemToCSR(pem):
    return x509.load_pem_x509_csr(pem.encode(), default_backend())


def keyToPEM(key):
    return key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.TraditionalOpenSSL,
        encryption_algorithm=serialization.NoEncryption()
    ).decode()


def pemToKey(pem):
    return serialization.load_pem_private_key(pem.encode(), password=None)
