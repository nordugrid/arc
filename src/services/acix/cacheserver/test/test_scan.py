import os

from twisted.trial import unittest
from twisted.internet import defer

from acix.cacheserver import pscan



class ScanTestCase(unittest.TestCase):

    CACHE_DIR1 = '/tmp/acix-test-cache/cache'
    CACHE_DIR2 = '/tmp/acix-test-cache/cache2'

    def setUp(self):
        pass


    @defer.inlineCallbacks
    def testScan(self):

        scanner = pscan.CacheScanner(self.CACHE_DIR1)
        l = []

        yield scanner.scan(lambda url : l.append(url))

        self.failUnlessIn('6b27f066ef9e22d2e3e40c668cae72e9e163fafd', l)
        self.failUnlessIn('a57c87cedbb464eb765a9fa8b8d506686cf0d0ee', l)
        self.failUnlessIn('eb4c0bc35aa6eefca27b84c8e40fb707c9de2218', l)

        self.failIfIn('abc', l)
        self.failIfIn('some_other_thing', l)


    @defer.inlineCallbacks
    def testScanMultipleDirs(self):

        scanner = pscan.CacheScanner([self.CACHE_DIR1, self.CACHE_DIR2])
        l = []

        yield scanner.scan(lambda url : l.append(url))

        # from the first dir
        self.failUnlessIn('a57c87cedbb464eb765a9fa8b8d506686cf0d0ee', l)
        self.failUnlessIn('eb4c0bc35aa6eefca27b84c8e40fb707c9de2218', l)
        # from the second dir
        self.failUnlessIn('9f4f96f6aada65ef3dafce1af2e36ba8428aeb03', l)
        self.failUnlessIn('dc294265ad76c92fe388f4f3c452734b10064ac2', l)

