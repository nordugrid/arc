"""
Client for retrieving cache.
"""

from twisted.python import log
from twisted.internet import reactor
from twisted.web import client

HEADER_HASHES = 'x-hashes'
HEADER_CACHE_TIME = 'x-cache-time'



class InvalidCacheReplyError(Exception):
    pass


def retrieveCache(url, contextFactory=None):
    # mostly copied from twisted.web.client
    """
    Returns a deferred, which will fire with a tuple
    consisting of a the hashes, generation-time, and the cache.
    """
    scheme, host, port, path = client._parse(url)
    factory = client.HTTPClientFactory(url)
    factory.noisy = False

    if scheme == 'https':
        from twisted.internet import ssl
        if contextFactory is None:
            contextFactory = ssl.ClientContextFactory()
        reactor.connectSSL(host, port, factory, contextFactory)
    else:
        reactor.connectTCP(host, port, factory)

    factory.deferred.addCallback(_gotCache, factory, url)
    return factory.deferred


def _gotCache(result, factory, url):
    log.msg("Got reply from cache service %s" % url)
    try:
        hashes = factory.response_headers[HEADER_HASHES]
        cache_time = factory.response_headers[HEADER_CACHE_TIME]
    except KeyError, e:
        raise InvalidCacheReplyError(str(e))

    #log.msg("Raw cache headers. Hashes: %s. Cache time: %s." % (hashes, cache_time))
    assert len(hashes) == 1, "Got more than one hash header"
    assert len(cache_time) == 1, "Got more than one cache time header"

    hashes = hashes[0].split(',')
    cache_time = float(cache_time[0])

    return hashes, cache_time, result

