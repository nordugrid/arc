"""
resource to fetch a bloom filter from
"""

import hashlib
import urlparse

from twisted.python import log
from twisted.internet import defer, task
from twisted.application import service

from acix.core import bloomfilter, cacheclient, ssl



class CacheIndex(service.Service):

    def __init__(self, urls, refresh_interval=300):
        self.urls = urls
        self.refresh_interval = refresh_interval # seconds
        self.index_task = task.LoopingCall(self.renewIndex)

        self.filters = {} # host -> filter mapping


    def startService(self):
        self.index_task.start(self.refresh_interval)


    def stopService(self):
        self.index_task.stop()


    def renewIndex(self):
        log.msg("Renewing index")

        dl = []

        cf = ssl.ContextFactory(verify=True)
        for url in self.urls:
            log.msg("Fetching cache from: " + url)
            d = cacheclient.retrieveCache(url, cf)
            d.addCallback(self._updateCache, url)
            d.addErrback(self._failedCacheRetrieval, url)
            dl.append(d)
        return defer.DeferredList(dl)


    def _updateCache(self, result, url):

        hashes, cache_time, cache = result

        host = urlparse.urlparse(url).netloc
        if ':' in host:
            host = host.split(':')[0]

        try:
            size = len(cache) * 8
            self.filters[host] = bloomfilter.BloomFilter(size=size, bits=cache, hashes=hashes)
        except Exception, e:
            log.err(e)
        log.msg("New cache added for " + host)


    def _failedCacheRetrieval(self, failure, url):
        log.msg("Failed to retrieve cache index from %s. Reason: %s" % (url, failure.getErrorMessage()))


    def query(self, keys):
        results = {}
        for host, filter_ in self.filters.items():
            for key in keys:
                khash = hashlib.sha1(key).hexdigest()
                hosts = results.setdefault(key, [])
                log.msg("Query: %s for %s" % (khash, host))
                if khash in filter_:
                    log.msg("Found match for %s at %s" % (key, host))
                    hosts.append(host)

        return results

