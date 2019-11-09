"""
Bloom Filter for Acix.

Heavily inspired from:
http://stackoverflow.com/questions/311202/modern-high-performance-bloom-filter-in-python
but modified to use bitarray instead of BitVector, as serialization
utterly sucks for the latter.

The calculateSize is more or less copied from pybloom (which also doesn't
support serialization and restore in a sensibile way.

The hash library is from:
http://www.partow.net/programming/hashfunctions/index.html
"""

from __future__ import print_function

import math

from acix.core import bitvector, hashes


# Note: These names are used to identify hashes used to generate a bloom
# filter between machines, i.e., they are used in the protocol.
# Do NOT change unless you are REALLY certain you know what you are doing
HASHES = {
    'rs'    : hashes.RSHash,
    'js'    : hashes.JSHash,
    'pjw'   : hashes.PJWHash,
    'elf'   : hashes.ELFHash,
    'bkdr'  : hashes.BKDRHash,
    'sdbm'  : hashes.SDBMHash,
    'djb'   : hashes.DJBHash,
    'dek'   : hashes.DEKHash,
    'bp'    : hashes.BPHash,
    'fnv'   : hashes.FNVHash,
}

# These hashes have been tested to be reasonably fast
# By all means try to avoid the rs hash, as it is awfully slow.
DEFAULT_HASHES = [ 'dek', 'elf', 'djb', 'sdbm' ]



def calculateSize(capacity, error_rate=0.001):
    slices = math.ceil(math.log(1 / error_rate, 2))
    # the error_rate constraint assumes a fill rate of 1/2
    # so we double the capacity to simplify the API
    bits = math.ceil( (2 * capacity * abs(math.log(error_rate))) /
                      (slices * (math.log(2) ** 2)) )

    size = int(slices * bits)

    ROUND_TO = 32
    # make sure we return a multiple of 32 (otherwise bitvector serialization will explode)
    if size % ROUND_TO != 0:
        mp = size // ROUND_TO + 1
        size = mp * ROUND_TO
    return size



class BloomFilter(object):
    def __init__(self, size=None, bits=None, hashes=None):

        self.size = size

        if bits is None:
            self.bits = bitvector.BitVector(size)
        else:
            assert size == len(bits) * 8, "Size and bit length does not match (%i,%i)" % (size, len(bits))
            self.bits = bitvector.BitVector(size, bits)

        self.used_hashes = []
        self.hashes = []

        if hashes is None:
            hashes = DEFAULT_HASHES[:]

        for hash in hashes:
            self.used_hashes.append(hash)
            self.hashes.append(HASHES[hash])


    def __contains__(self, key):
        for i in self._indexes(key):
                if not self.bits[i]:
                        return False
        return True


    def add(self, key):
        for i in self._indexes(key):
            self.bits[i] = 1


    def _indexes(self, key):
        ords = [ ord(c) for c in key ]
        return [ hash(ords) % self.size for hash in self.hashes ]


    def get_hashes(self):
        return self.used_hashes[:]


    def serialize(self):
        return self.bits.tostring()



if __name__ == '__main__':
    import time
    from acix.scanner import pscan

    try:
        scanner = pscan.CacheScanner()
    except IOError:
        scanner = pscan.CacheScanner('test/cache')

    bf = BloomFilter(1000672)

    t0 = time.time()
    scanner.scan(bf.add)
    td = time.time() - t0
    print("Time taken for bloom filter build: %s" % td)

