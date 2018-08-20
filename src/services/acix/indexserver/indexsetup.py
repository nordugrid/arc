import os

from twisted.application import internet, service
from twisted.web import resource, server

from acix.core import ssl
from acix.indexserver import index, indexresource

from arc.utils import config

# -- constants
SSL_DEFAULT = True
INDEX_TCP_PORT = 6080
INDEX_SSL_PORT = 6443
DEFAULT_INDEX_REFRESH_INTERVAL = 301  # seconds between updating cache
ARC_CONF = '/etc/arc.conf'
DEFAULT_HOST_KEY  = '/etc/grid-security/hostkey.pem'
DEFAULT_HOST_CERT = '/etc/grid-security/hostcert.pem'
DEFAULT_CERTIFICATES = '/etc/grid-security/certificates'


def getCacheConf():

    '''Return a tuple of (cachescanners, x509_host_key, x509_host_cert, x509_cert_dir)'''
    config.parse_arc_conf(os.environ['ARC_CONFIG'] if 'ARC_CONFIG' in os.environ else ARC_CONF)
    cachescanners = config.get_value('cachescanner', 'acix-index', force_list=True)
    x509_host_key = config.get_value('x509_host_key', ['acix-scanner', 'common']) or DEFAULT_HOST_KEY
    x509_host_cert = config.get_value('x509_host_cert', ['acix-scanner', 'common']) or DEFAULT_HOST_CERT
    x509_cert_dir = config.get_value('x509_cert_dir', ['acix-scanner', 'common']) or DEFAULT_CERTIFICATES

    return (cachescanners, x509_host_key, x509_host_cert, x509_cert_dir)

def createIndexApplication(use_ssl=SSL_DEFAULT, port=None,
                           refresh_interval=DEFAULT_INDEX_REFRESH_INTERVAL):

    # monkey-patch fix for dealing with low url-length limit
    from twisted.protocols import basic
    basic.LineReceiver.MAX_LENGTH = 65535

    cachescanners, x509_host_key, x509_host_cert, x509_cert_dir = getCacheConf()
    if not cachescanners:
        return None

    ci = index.CacheIndex(cachescanners, refresh_interval)

    siteroot = resource.Resource()
    dataroot = resource.Resource()

    dataroot.putChild(b'index', indexresource.IndexResource(ci))
    siteroot.putChild(b'data', dataroot)

    site = server.Site(siteroot)

    application = service.Application("arc-indexserver")

    ci.setServiceParent(application)

    if use_ssl:
        cf = ssl.ContextFactory(key_path=x509_host_key, cert_path=x509_host_cert, ca_dir=x509_cert_dir)
        internet.SSLServer(port or INDEX_SSL_PORT, site, cf).setServiceParent(application)
    else:
        internet.TCPServer(port or INDEX_TCP_PORT, site).setServiceParent(application)

    return application

