import os
import shutil
import tempfile

from twisted.trial import unittest
from twisted.internet import defer

from acix.scanner import pscan



class ScanTestCase(unittest.TestCase):

    def setUp(self):
        # fill caches
        self.tmpdir = tempfile.mkdtemp(prefix='/tmp/acix-test-cache') 
        os.mkdir(self.tmpdir+'/cache')
        os.mkdir(self.tmpdir+'/cache/data')
        os.mkdir(self.tmpdir+'/cache/data/6b')
        f = open(self.tmpdir+'/cache/data/6b/27f066ef9e22d2e3e40c668cae72e9e163fafd.meta', 'w')
        f.write('http://localhost/file1')
        f.close()
        f = open(self.tmpdir+'/cache/data/6b/27f066ef9e22d2e3e40c668cae72e9e163fafd', 'w')
        f.write('1234')
        f.close()
        os.mkdir(self.tmpdir+'/cache/data/a5')
        f = open(self.tmpdir+'/cache/data/a5/7c87cedbb464eb765a9fa8b8d506686cf0d0ee.meta', 'w')
        f.write('http://localhost/file2')
        f.close()
        f = open(self.tmpdir+'/cache/data/a5/7c87cedbb464eb765a9fa8b8d506686cf0d0ee', 'w')
        f.write('1234')
        f.close()
        
        self.tmpdir2 = tempfile.mkdtemp(prefix='/tmp/acix-test-cache2') 
        os.mkdir(self.tmpdir2+'/cache')
        os.mkdir(self.tmpdir2+'/cache/data')
        os.mkdir(self.tmpdir2+'/cache/data/9f')
        f = open(self.tmpdir2+'/cache/data/9f/4f96f6aada65ef3dafce1af2e36ba8428aeb03.meta', 'w')
        f.write('http://localhost/file3')
        f.close()
        f = open(self.tmpdir2+'/cache/data/9f/4f96f6aada65ef3dafce1af2e36ba8428aeb03', 'w')
        f.write('1234')
        f.close()
        os.mkdir(self.tmpdir2+'/cache/data/dc')
        f = open(self.tmpdir2+'/cache/data/dc/294265ad76c92fe388f4f3c452734b10064ac2.meta', 'w')
        f.write('http://localhost/file4')
        f.close()
        f = open(self.tmpdir2+'/cache/data/dc/294265ad76c92fe388f4f3c452734b10064ac2', 'w')
        f.write('1234')
        f.close()

    def tearDown(self):
        shutil.rmtree(self.tmpdir)
        shutil.rmtree(self.tmpdir2)

    @defer.inlineCallbacks
    def testScan(self):

        scanner = pscan.CacheScanner([self.tmpdir+'/cache'])
        l = []

        yield scanner.scan(lambda url : l.append(url.decode()))

        self.failUnlessIn('6b27f066ef9e22d2e3e40c668cae72e9e163fafd', l)
        self.failUnlessIn('a57c87cedbb464eb765a9fa8b8d506686cf0d0ee', l)

        self.failIfIn('abc', l)
        self.failIfIn('some_other_thing', l)


    @defer.inlineCallbacks
    def testScanMultipleDirs(self):

        scanner = pscan.CacheScanner([self.tmpdir+'/cache', self.tmpdir2+'/cache'])
        l = []

        yield scanner.scan(lambda url : l.append(url.decode()))

        # from the first dir
        self.failUnlessIn('a57c87cedbb464eb765a9fa8b8d506686cf0d0ee', l)
        self.failUnlessIn('6b27f066ef9e22d2e3e40c668cae72e9e163fafd', l)
        # from the second dir
        self.failUnlessIn('9f4f96f6aada65ef3dafce1af2e36ba8428aeb03', l)
        self.failUnlessIn('dc294265ad76c92fe388f4f3c452734b10064ac2', l)

