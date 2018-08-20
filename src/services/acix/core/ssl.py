from __future__ import print_function

import os

from OpenSSL import SSL


DEFAULT_HOST_KEY  = '/etc/grid-security/hostkey.pem'
DEFAULT_HOST_CERT = '/etc/grid-security/hostcert.pem'
DEFAULT_CERTIFICATES = '/etc/grid-security/certificates'


class ContextFactory(object):

    def __init__(self, key_path=DEFAULT_HOST_KEY, cert_path=DEFAULT_HOST_CERT,
                 verify=False, ca_dir=None):

        self.key_path = key_path
        self.cert_path = cert_path
        self.verify = verify
        self.ca_dir = ca_dir
        if self.verify and ca_dir is None:
            self.ca_dir = DEFAULT_CERTIFICATES
        self.ctx = None


    def getContext(self):
        if self.ctx is not None:
            return self.ctx

        ctx = SSL.Context(SSL.SSLv23_METHOD) # this also allows tls 1.0
        ctx.set_options(SSL.OP_NO_SSLv2) # ssl2 is unsafe
        ctx.set_options(SSL.OP_NO_SSLv3) # ssl3 is also unsafe

        ctx.use_privatekey_file(self.key_path)
        ctx.use_certificate_file(self.cert_path)
        ctx.check_privatekey() # sanity check

        def verify_callback(conn, x509, error_number, error_depth, allowed):
            # just return what openssl thinks is right
            return allowed

        if self.verify:
            ctx.set_verify(SSL.VERIFY_PEER, verify_callback)

            calist = [ ca for ca in os.listdir(self.ca_dir) if ca.endswith('.0') ]
            for ca in calist:
                # openssl wants absolute paths
                ca = os.path.join(self.ca_dir, ca)
                ctx.load_verify_locations(ca)

        if self.ctx is None:
            self.ctx = ctx

        return ctx



if __name__ == '__main__':
    cf = ContextFactory()
    ctx = cf.getContext()
    print(ctx)

