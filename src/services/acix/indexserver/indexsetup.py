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

def getCacheServers():

    config.parse_arc_conf(os.environ['ARC_CONFIG'] if 'ARC_CONFIG' in os.environ else ARC_CONF)
    cachescanners = config.get_value('cachescanner', 'acix-index', force_list=True)
    # use localhost as default if none defined
    if not cachescanners:
        cachescanners = ['https://localhost:5443/data/cache']
    return cachescanners

def createIndexApplication(use_ssl=SSL_DEFAULT, port=None,
                           refresh_interval=DEFAULT_INDEX_REFRESH_INTERVAL):

    # monkey-patch fix for dealing with low url-length limit
    from twisted.protocols import basic
    basic.LineReceiver.MAX_LENGTH = 65535

    ci = index.CacheIndex(getCacheServers(), refresh_interval)

    siteroot = resource.Resource()
    dataroot = resource.Resource()

    dataroot.putChild(b'index', indexresource.IndexResource(ci))
    siteroot.putChild(b'data', dataroot)

    site = server.Site(siteroot)

    application = service.Application("arc-indexserver")

    ci.setServiceParent(application)

    if use_ssl:
        cf = ssl.ContextFactory()
        internet.SSLServer(port or INDEX_SSL_PORT, site, cf).setServiceParent(application)
    else:
        internet.TCPServer(port or INDEX_TCP_PORT, site).setServiceParent(application)

    return application

