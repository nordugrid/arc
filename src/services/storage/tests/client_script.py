#! /usr/bin/env python
import sys, time
import storage.client
try:
    start = int(sys.argv[1])
    end = int(sys.argv[2])
except:
    print sys.argv[0], '<start> <end>'
    sys.exit(1)
a = storage.client.AHashClient('http://localhost:60002/AHash')
for i in range(start, end):
    print i,
    change_start = time.time()
    a.change({i: ['0', 'set', 'entries', i, '0', {}]})
    print "%0.5f" % (time.time()-change_start),
    get_start = time.time()
    a.get(['0'])
    print "%0.5f" % (time.time()-get_start)
