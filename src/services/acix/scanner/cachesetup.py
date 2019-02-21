import os
import socket

from twisted.application import internet, service
from twisted.web import resource, server

from acix.core import ssl
from acix.scanner import pscan, cache, cacheresource

from arc.utils import config

# -- constants
SSL_DEFAULT = True
CACHE_INTERFACE = ''
CACHE_TCP_PORT = 5080
CACHE_SSL_PORT = 5443
DEFAULT_CAPACITY = 30000              # number of files in cache
DEFAULT_CACHE_REFRESH_INTERVAL = 600  # seconds between updating cache
DEFAULT_HOST_KEY  = '/etc/grid-security/hostkey.pem'
DEFAULT_HOST_CERT = '/etc/grid-security/hostcert.pem'
DEFAULT_CERTIFICATES = '/etc/grid-security/certificates'

DEFAULT_WS_INTERFACE = 'https://hostname:443/arex' # in case not defined in arc.conf
ARC_CONF = '/etc/arc.conf'

def getCacheConf():
    '''Return a tuple of (cache_url, cache_dump, cache_host, cache_port, x509_host_key, x509_host_cert, x509_cert_dir)'''

    config.parse_arc_conf(os.environ['ARC_CONFIG'] if 'ARC_CONFIG' in os.environ else ARC_CONF)

    # Use cache access URL if [arex/ws/cache] is present
    cache_url = ''
    if config.check_blocks('arex/ws/cache'):
        arex_url = config.get_value('wsurl', 'arex/ws')
        if not arex_url:
            # Use default endpoint, but first we need hostname
            hostname = config.get_value('hostname', 'common') or socket.gethostname()
            arex_url = DEFAULT_WS_INTERFACE.replace('hostname', hostname)
        cache_url = '%s/cache' % arex_url

    cache_dump = config.get_value('cachedump', 'acix-scanner') == 'yes'
    cache_host = config.get_value('hostname', 'acix-scanner') or CACHE_INTERFACE
    cache_port = int(config.get_value('port', 'acix-scanner') or CACHE_SSL_PORT)

    x509_host_key = config.get_value('x509_host_key', ['acix-scanner', 'common']) or DEFAULT_HOST_KEY
    x509_host_cert = config.get_value('x509_host_cert', ['acix-scanner', 'common']) or DEFAULT_HOST_CERT
    x509_cert_dir = config.get_value('x509_cert_dir', ['acix-scanner', 'common']) or DEFAULT_CERTIFICATES

    return (cache_url, cache_dump, cache_host, cache_port, x509_host_key, x509_host_cert, x509_cert_dir)


def createCacheApplication(use_ssl=SSL_DEFAULT, port=None, cache_dir=None,
                           capacity=DEFAULT_CAPACITY, refresh_interval=DEFAULT_CACHE_REFRESH_INTERVAL):

    (cache_url, cache_dump, cache_host, cache_port, x509_host_key, x509_host_cert, x509_cert_dir) = getCacheConf()

    scanner = pscan.CacheScanner(cache_dir, cache_dump)
    cs = cache.Cache(scanner, capacity, refresh_interval, cache_url)

    cr = cacheresource.CacheResource(cs)

    siteroot = resource.Resource()
    dataroot = resource.Resource()

    dataroot.putChild(b'cache', cr)
    siteroot.putChild(b'data', dataroot)

    site = server.Site(siteroot)

    # setup application
    application = service.Application("acix-scanner")

    cs.setServiceParent(application)

    if use_ssl:
        cf = ssl.ContextFactory(key_path=x509_host_key, cert_path=x509_host_cert, ca_dir=x509_cert_dir)
        internet.SSLServer(port or cache_port, site, cf, interface=cache_host).setServiceParent(application)
    else:
        internet.TCPServer(port or CACHE_TCP_PORT, site, interface=cache_host).setServiceParent(application)

    return application

