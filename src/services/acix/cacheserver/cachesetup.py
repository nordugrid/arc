import os

from twisted.application import internet, service
from twisted.web import resource, server

from acix.core import ssl
from acix.cacheserver import pscan, cache, cacheresource


# -- constants
SSL_DEFAULT = True
CACHE_TCP_PORT = 5080
CACHE_SSL_PORT = 5443
DEFAULT_CAPACITY = 30000              # number of files in cache
DEFAULT_CACHE_REFRESH_INTERVAL = 600  # seconds between updating cache

ARC_CONF = '/etc/arc.conf'

def getCacheConf():
    '''Return a tuple of (cache_url, cache_dump)'''

    # Use cache access URL if mount point and at least one cacheaccess is defined
    cache_url = ''
    cache_dump = False
    config = ARC_CONF
    if 'ARC_CONFIG' in os.environ:
        config = os.environ['ARC_CONFIG']
    cacheaccess = False
    for line in file(config):
        if line.startswith('arex_mount_point'):
            args = line.split('=', 2)[1]
            url = args.replace('"', '').strip()
            cache_url = url + '/cache'
        if line.startswith('cacheaccess'):
            cacheaccess = True
        if line.startswith('cachedump') and line.find('yes') != -1:
            cache_dump = True
    if not cacheaccess:
        cache_url = ''
    return (cache_url, cache_dump)


def createCacheApplication(use_ssl=SSL_DEFAULT, port=None, cache_dir=None,
                           capacity=DEFAULT_CAPACITY, refresh_interval=DEFAULT_CACHE_REFRESH_INTERVAL):

    (cache_url, cache_dump) = getCacheConf()

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
        internet.SSLServer(port or CACHE_SSL_PORT, site, cf).setServiceParent(application)
    else:
        internet.TCPServer(port or CACHE_TCP_PORT, site).setServiceParent(application)

    return application

