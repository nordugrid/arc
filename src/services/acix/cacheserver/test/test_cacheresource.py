from twisted.trial import unittest

from twisted.internet import reactor, defer
from twisted.web import resource, server

from acix.core import bloomfilter, cacheclient
from acix.cacheserver import cache, cacheresource


TEST_URLS1 = [ 'srm://srm.ndgf.org/biogrid/db/uniprot/UniProt12.6/uniprot_sprot.fasta.gz',
               'gsiftp://grid.tsl.uu.se:2811/storage/sam/testfile']


class TestScanner(object):

    def __init__(self, urls):
        self.urls = urls

    def dir(self):
        return "testscanner (no dir)"

    def scan(self, filter):
        for url in self.urls:
            filter(url)
        d = defer.Deferred()
        d.callback(None)
        return d


class CacheResourceTest(unittest.TestCase):

    port = 4080

    @defer.inlineCallbacks
    def setUp(self):

        scanner = TestScanner(TEST_URLS1)

        self.cs = cache.Cache(scanner, 10000, 60, '')
        cr = cacheresource.CacheResource(self.cs)

        siteroot = resource.Resource()
        siteroot.putChild(b'cache', cr)
        site = server.Site(siteroot)

        yield self.cs.startService()

        self.iport = reactor.listenTCP(self.port, site)

        self.cache_url = 'http://localhost:%i/cache' % (self.port)


    @defer.inlineCallbacks
    def tearDown(self):
        yield self.cs.stopService()
        yield self.iport.stopListening()


    @defer.inlineCallbacks
    def testCacheRetrieval(self):

        hashes, cache_time, cache, cache_url = yield cacheclient.retrieveCache(self.cache_url)

        size = len(cache) * 8
        bf = bloomfilter.BloomFilter(size, bits=cache, hashes=hashes)

        for url in TEST_URLS1:
            self.assertTrue(url in bf)

        self.assertFalse("gahh" in bf)
        self.assertFalse("whuu" in bf)

