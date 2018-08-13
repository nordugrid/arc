import os
import socket

from twisted.application import internet, service
from twisted.web import resource, server

from acix.core import ssl
from acix.cacheserver import pscan, cache, cacheresource

from arc.utils import config

# -- constants
SSL_DEFAULT = True
CACHE_INTERFACE = ''
CACHE_TCP_PORT = 5080
CACHE_SSL_PORT = 5443
DEFAULT_CAPACITY = 30000              # number of files in cache
DEFAULT_CACHE_REFRESH_INTERVAL = 600  # seconds between updating cache

DEFAULT_WS_INTERFACE = 'https://hostname:443/arex' # in case not defined in arc.conf
ARC_CONF = '/etc/arc.conf'

def getCacheConf():
    '''Return a tuple of (cache_url, cache_dump, cache_host, cache_port)'''

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
    
    return (cache_url, cache_dump, cache_host, cache_port)


def createCacheApplication(use_ssl=SSL_DEFAULT, port=None, cache_dir=None,
                           capacity=DEFAULT_CAPACITY, refresh_interval=DEFAULT_CACHE_REFRESH_INTERVAL):

    (cache_url, cache_dump, cache_host, cache_port) = getCacheConf()

    scanner = pscan.CacheScanner(cache_dir, cache_dump)
    cs = cache.Cache(scanner, capacity, refresh_interval, cache_url)

    cr = cacheresource.CacheResource(cs)

    siteroot = resource.Resource()
    dataroot = resource.Resource()

    dataroot.putChild('cache', cr)
    siteroot.putChild('data', dataroot)

    site = server.Site(siteroot)

    # setup application
    application = service.Application("arc-cacheserver")

    cs.setServiceParent(application)

    if use_ssl:
        cf = ssl.ContextFactory()
        internet.SSLServer(port or cache_port, site, cf, interface=cache_host).setServiceParent(application)
    else:
        internet.TCPServer(port or CACHE_TCP_PORT, site, interface=cache_host).setServiceParent(application)

    return application

