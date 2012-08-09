import os

from twisted.application import internet, service
from twisted.web import resource, server

from acix.core import ssl
from acix.indexserver import index, indexresource


# -- constants
SSL_DEFAULT = True
INDEX_TCP_PORT = 6080
INDEX_SSL_PORT = 6443
DEFAULT_INDEX_REFRESH_INTERVAL = 301  # seconds between updating cache
ARC_CONF = '/etc/arc.conf'

def getCacheServers():

    cache_dirs = []
    config = ARC_CONF
    if 'ARC_CONFIG' in os.environ:
        config = os.environ['ARC_CONFIG']
    for line in file(config):
        if line.startswith('cacheserver'):
            args = line.split('=', 2)[1]
            cache_dir = args.replace('"', '').strip()
            cache_dirs.append(cache_dir)
    # use localhost as default if none defined
    if len(cache_dirs) == 0:
        cache_dirs.append('https://localhost:5443/data/cache')
    return cache_dirs


def createIndexApplication(use_ssl=SSL_DEFAULT, port=None,
                           refresh_interval=DEFAULT_INDEX_REFRESH_INTERVAL):

    # monkey-patch fix for dealing with low url-length limit
    from twisted.protocols import basic
    basic.LineReceiver.MAX_LENGTH = 65535

    ci = index.CacheIndex(getCacheServers(), refresh_interval)

    siteroot = resource.Resource()
    dataroot = resource.Resource()

    dataroot.putChild('index', indexresource.IndexResource(ci))
    siteroot.putChild('data', dataroot)

    site = server.Site(siteroot)

    application = service.Application("arc-indexserver")

    ci.setServiceParent(application)

    if use_ssl:
        cf = ssl.ContextFactory()
        internet.SSLServer(port or INDEX_SSL_PORT, site, cf).setServiceParent(application)
    else:
        internet.TCPServer(port or INDEX_TCP_PORT, site).setServiceParent(application)

    return application

