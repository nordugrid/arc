"""
setup two cache resources, and one index resources,
and try out the whole things and see if it works :-)
"""

import hashlib

from twisted.trial import unittest

from twisted.internet import reactor, defer
from twisted.web import resource, server

from acix.core import indexclient
from acix.scanner import cache, cacheresource
from acix.indexserver import index, indexresource


TEST_URLS1 = [ 'srm://srm.ndgf.org/pnfs/ndgf.org/data/ops/sam-test/testfile',
               'gsiftp://grid.tsl.uu.se:2811/storage/sam/testfile']

TEST_URLS2 = [ 'lfc://lfc1.ndgf.org//grid/ops.ndgf.org/sam/testfile',
               'srm://srm.ndgf.org/pnfs/ndgf.org/data/ops/sam-test/testfile']


class TestScanner(object):

    def __init__(self, urls):
        self.urls = urls

    def dir(self):
        return "testscanner (no dir)"

    def scan(self, filter):
        for url in self.urls:
            filter(hashlib.sha1(url.encode()).hexdigest())
        return defer.succeed(None)


class SystemTest(unittest.TestCase):

    cport1 = 4080
    cport2 = 4081
    xport  = 4082

    @defer.inlineCallbacks
    def setUp(self):

        # cheap trick to get multiple hostnames on one host
        self.cache_urls = [ 'http://localhost:%i/cache' % self.cport1,
                            'http://127.0.0.1:%i/cache' % self.cport2 ]

        scanner1 = TestScanner(TEST_URLS1)
        scanner2 = TestScanner(TEST_URLS2)

        self.cs1 = cache.Cache(scanner1, 10000, 60, '')
        self.cs2 = cache.Cache(scanner2, 10000, 60, 'http://127.0.0.1/arex/cache')
        self.idx = index.CacheIndex(self.cache_urls)

        cr1 = cacheresource.CacheResource(self.cs1)
        cr2 = cacheresource.CacheResource(self.cs2)
        idxr = indexresource.IndexResource(self.idx)

        c1siteroot = resource.Resource()
        c1siteroot.putChild(b'cache', cr1)
        c1site = server.Site(c1siteroot)

        c2siteroot = resource.Resource()
        c2siteroot.putChild(b'cache', cr2)
        c2site = server.Site(c2siteroot)

        idx_siteroot = resource.Resource()
        idx_siteroot.putChild(b'index', idxr)
        idx_site = server.Site(idx_siteroot)

        yield self.cs1.startService()
        yield self.cs2.startService()

        self.iport1 = reactor.listenTCP(self.cport1, c1site)
        self.iport2 = reactor.listenTCP(self.cport2, c2site)

        #yield self.idx.startService()
        yield self.idx.renewIndex() # ensure that we have fetched cache
        self.iport3 = reactor.listenTCP(self.xport, idx_site)


        self.index_url = "http://localhost:%i/index" % (self.xport)


    @defer.inlineCallbacks
    def tearDown(self):
        yield self.cs1.stopService()
        yield self.cs2.stopService()
        #yield self.idx.stopService()

        yield self.iport1.stopListening()
        yield self.iport2.stopListening()
        yield self.iport3.stopListening()


    @defer.inlineCallbacks
    def testIndexQuery(self):

        urls1 = [ TEST_URLS1[1] ]
        result = yield indexclient.queryIndex(self.index_url, urls1)
        self.failUnlessIn(urls1[0], result)
        locations = result[urls1[0]]
        self.failUnlessEqual(locations, ['localhost'])

        urls2 = [ TEST_URLS1[0] ]
        result = yield indexclient.queryIndex(self.index_url, urls2)
        self.failUnlessIn(urls2[0], result)
        locations = result[urls2[0]]
        self.failUnlessEqual(len(locations), 2)
        self.failUnlessIn('localhost', locations)
        self.failUnlessIn('http://127.0.0.1/arex/cache', locations)

        urls3 = [ 'srm://host/no_such_file' ]
        result = yield indexclient.queryIndex(self.index_url, urls3)
        self.failUnlessIn(urls3[0], result)
        self.failUnlessEqual(result, {urls3[0]: []})

        urls4 = [ TEST_URLS2[0] ]
        result = yield indexclient.queryIndex(self.index_url, urls4)
        self.failUnlessIn(urls4[0], result)
        locations = result[urls4[0]]
        self.failUnlessEqual(locations, ['http://127.0.0.1/arex/cache'])

