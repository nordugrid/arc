"""
resource to fetch a bloom filter from
"""

import time

from twisted.python import log
from twisted.internet import task
from twisted.application import service

from acix.core import bloomfilter


CAPACITY_CHUNK = 10000  # 10k entries is the least we bother with
WATERMARK_LOW = 10000  # 10k entries -> 35k memory, no reason to go lower


class _Counter(object):

    def __init__(self):
        self.n = 0

    def up(self):
        self.n += 1


class Cache(service.Service):

    def __init__(self, scanner, capacity, refresh_interval, cache_url):

        self.scanner = scanner
        self.capacity = capacity
        self.refresh_interval = refresh_interval
        self.cache_url = cache_url

        self.cache_task = task.LoopingCall(self.renewCache)

        self.cache = None
        self.generation_time = None
        self.hashes = []

    def startService(self):
        log.msg("-" * 60)
        log.msg("Starting cache service")
        log.msg("  Directory        : %s" % self.scanner.dir())
        log.msg("  Capacity         : %s" % self.capacity)
        log.msg("  Refresh interval : %i" % self.refresh_interval)
        log.msg("-" * 60)

        self.cache_task.start(self.refresh_interval)

    def stopService(self):
        self.cache_task.stop()

    def renewCache(self):
        n_bits = bloomfilter.calculateSize(capacity=self.capacity)

        log.msg("Renewing cache. Filter capacity %i, size: %i bits" % (self.capacity, n_bits))
        filter = bloomfilter.BloomFilter(n_bits)

        file_counter = _Counter()

        def addEntry(key):
            file_counter.up()
            filter.add(key)

        t0 = time.time()
        d = self.scanner.scan(addEntry)
        d.addCallback(self._scanDone, filter, t0, file_counter)
        return d

    def _scanDone(self, _, filter, t0, file_counter):
        td = time.time() - t0

        self.cache = filter.serialize()
        self.generation_time = time.time()
        self.hashes = filter.get_hashes()

        log.msg("Cache updated. Time taken: %f seconds. Entries: %i" % (round(td, 2), file_counter.n))
        if file_counter.n == 0:
            log.msg("No file entries registered. Possible misconfiguration.")
            return

        self.checkCapacity(file_counter.n)

    def checkCapacity(self, n_files):

        if n_files > self.capacity:
            log.msg("Filter capacity exceeded. Capacity: %i. Files: %i" % (self.capacity, n_files))
            self.capacity = (round(n_files / float(CAPACITY_CHUNK)) + 1) * CAPACITY_CHUNK
            log.msg("Filter capacity expanded to %i (will take effect on next cache run)" % self.capacity)
            return

        if self.capacity / float(n_files) > 3.0 and self.capacity > WATERMARK_LOW:
            # filter under 1/3 full
            log.msg("Filter capacity underutilized. Capacity: %i. Files: %i" % (self.capacity, n_files))
            self.capacity = max(self.capacity - CAPACITY_CHUNK, WATERMARK_LOW)
            log.msg("Filter capacity reduced to %i (will take effect on next cache run)" % self.capacity)

    def getCache(self):

        return self.generation_time, self.hashes, self.cache, self.cache_url
