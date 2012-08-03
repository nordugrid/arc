from twisted.trial import unittest

from arc.cacheindex.core import bloomfilter

KEYS = ['one', 'two', 'three', 'four']
FALSE_KEYS = ['five', 'six', 'seven' ]
SIZE = 160



class BloomFilterTestCase(unittest.TestCase):

    def setUp(self):
        self.bf = bloomfilter.BloomFilter(SIZE)


    def testContains(self):

        for key in KEYS: self.bf.add(key)

        for key in KEYS: self.failUnlessIn(key, self.bf)
        for key in FALSE_KEYS: self.failIfIn(key, self.bf)


    def testSerialization(self):

        for key in KEYS: self.bf.add(key)

        s = self.bf.serialize()
        bf2 = bloomfilter.BloomFilter(SIZE, s)

        for key in KEYS: self.failUnlessIn(key, bf2)
        for key in FALSE_KEYS: self.failIfIn(key, bf2)


    def testReconstruction(self):

        # create filter with some non-standard hashes...
        bf1 = bloomfilter.BloomFilter(SIZE, hashes=['js', 'dek', 'sdbm'])
        for key in KEYS: bf1.add(key)

        # just to be sure
        for key in KEYS: self.failUnlessIn(key, bf1)
        for key in FALSE_KEYS: self.failIfIn(key, bf1)

        # reconstruct
        bf2 = bloomfilter.BloomFilter(SIZE, bits=bf1.serialize(), hashes=bf1.get_hashes())

        for key in KEYS: self.failUnlessIn(key, bf2)
        for key in FALSE_KEYS: self.failIfIn(key, bf2)

