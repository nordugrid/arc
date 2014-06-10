"""
Client for retrieving cache.
"""

from urlparse import urlparse

from twisted.python import log
from twisted.internet import reactor
from twisted.web import client

HEADER_HASHES = 'x-hashes'
HEADER_CACHE_TIME = 'x-cache-time'
HEADER_CACHE_URL = 'x-cache-url'



class InvalidCacheReplyError(Exception):
    pass


def retrieveCache(url, contextFactory=None):
    # mostly copied from twisted.web.client
    """
    Returns a deferred, which will fire with a tuple
    consisting of a the hashes, generation-time, and the cache.
    """
    u = urlparse(url)
    factory = client.HTTPClientFactory(url)
    factory.noisy = False

    if u.scheme == 'https':
        from twisted.internet import ssl
        if contextFactory is None:
            contextFactory = ssl.ClientContextFactory()
        reactor.connectSSL(u.hostname, u.port, factory, contextFactory)
    else:
        reactor.connectTCP(u.hostname, u.port, factory)

    factory.deferred.addCallback(_gotCache, factory, url)
    return factory.deferred


def _gotCache(result, factory, url):
    log.msg("Got reply from cache service %s" % url)
    try:
        hashes = factory.response_headers[HEADER_HASHES]
        cache_time = factory.response_headers[HEADER_CACHE_TIME]
    except KeyError, e:
        raise InvalidCacheReplyError(str(e))

    try:
        cache_url = factory.response_headers[HEADER_CACHE_URL][0]
    except KeyError, e: # Site may not expose cache to outside
        cache_url = ''
        
    #log.msg("Raw cache headers. Hashes: %s. Cache time: %s." % (hashes, cache_time))
    assert len(hashes) == 1, "Got more than one hash header"
    assert len(cache_time) == 1, "Got more than one cache time header"

    hashes = hashes[0].split(',')
    cache_time = float(cache_time[0])

    return hashes, cache_time, result, cache_url

