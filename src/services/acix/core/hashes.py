#
#**************************************************************************
#*                                                                        *
#*          General Purpose Hash Function Algorithms Library              *
#*                                                                        *
#* Author: Arash Partow - 2002                                            *
#* URL: http://www.partow.net                                             *
#* URL: http://www.partow.net/programming/hashfunctions/index.html        *
#*                                                                        *
#* Modified by Henrik Thostrup Jensen <htj@ndgf.org> to operate on int    *
#* arrays instead of strings (large optimization when performing several  *
#* hashes of the same string. (2009)                                      *
#*                                                                        *
#* Copyright notice:                                                      *
#* Free use of the General Purpose Hash Function Algorithms Library is    *
#* permitted under the guidelines and in accordance with the most current *
#* version of the Common Public License.                                  *
#* http://www.opensource.org/licenses/cpl.php                             *
#*                                                                        *
#**************************************************************************
#

import sys

if sys.version_info[0] >= 3:
    long = int

def RSHash(key):
    a    = 378551
    b    =  63689
    hash =      0
    for k in key:
      hash = hash * a + k
      a = a * b
    return hash


def JSHash(key):
    hash = 1315423911
    for k in key:
      hash ^= ((hash << 5) + k + (hash >> 2))
    return hash


def PJWHash(key):
   BitsInUnsignedInt = 4 * 8
   ThreeQuarters     = long((BitsInUnsignedInt * 3) // 4)
   OneEighth         = long(BitsInUnsignedInt // 8)
   HighBits          = (0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth)
   hash              = 0
   test              = 0

   for k in key:
     hash = (hash << OneEighth) + k
     test = hash & HighBits
     if test != 0:
       hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
   return (hash & 0x7FFFFFFF)


def ELFHash(key):
    hash = 0
    x    = 0
    for k in key:
      hash = (hash << 4) + k
      x = hash & 0xF0000000
      if x != 0:
        hash ^= (x >> 24)
      hash &= ~x
    return hash


def BKDRHash(key):
    seed = 131 # 31 131 1313 13131 131313 etc..
    hash = 0
    for k in key:
      hash = (hash * seed) + k
    return hash


def SDBMHash(key):
    hash = 0
    for k in key:
      hash = k + (hash << 6) + (hash << 16) - hash;
    return hash


def DJBHash(key):
    hash = 5381
    for k in key:
       hash = ((hash << 5) + hash) + k
    return hash


def DEKHash(key):
    hash = len(key);
    for k in key:
      hash = ((hash << 5) ^ (hash >> 27)) ^ k
    return hash


def BPHash(key):
    hash = 0
    for k in key:
       hash = hash << 7 ^ k
    return hash


def FNVHash(key):
    fnv_prime = 0x811C9DC5
    hash = 0
    for k in key:
      hash *= fnv_prime
      hash ^= k
    return hash


## requres index, so we don't use it
#def APHash(key):
#    hash = 0xAAAAAAAA
#    for k in key:
#      if ((i & 1) == 0):
#        hash ^= ((hash <<  7) ^ k * (hash >> 3))
#      else:
#        hash ^= (~((hash << 11) + k ^ (hash >> 5)))
#    return hash


