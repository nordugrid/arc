"""
Custom bitvector implementation, as most other suck.

Both BitVector and bitarray has problems with serialization,
which is rather critical for us.

There might be endian issues.

Author: Henrik Thostrup Jensen <htj@ndgf.org>
"""

import array


ARRAY_TYPE = 'B'
TYPE_SIZE = 8



class BitVector(object):

    def __init__(self, n_bits, bits=None):
        assert n_bits % TYPE_SIZE == 0, "Size must be a multiple of %i" % TYPE_SIZE
        if bits is None:
            self.bits = array.array(ARRAY_TYPE, [0] * (n_bits // TYPE_SIZE))
        else:
            assert n_bits == len(bits) * TYPE_SIZE, "Size and given bits does not match"
            self.bits = array.array(ARRAY_TYPE)
            try:
                self.bits.frombytes(bits)
            except AttributeError:
                self.bits.fromstring(bits)


    def __setitem__(self, index, value):
        assert value == 1, "Only possible to set bits"
        l = self.bits[index // TYPE_SIZE]
        self.bits[index // TYPE_SIZE] = l | 1 << (index % TYPE_SIZE)


    def __getitem__(self, index):
        l = self.bits[index // TYPE_SIZE]
        return (l >> (index % TYPE_SIZE)) & 1


    def tostring(self):
        try:
            return self.bits.tobytes()
        except AttributeError:
            return self.bits.tostring()
