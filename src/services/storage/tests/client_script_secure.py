#! /usr/bin/env python
import sys, time
import storage.client
try:
    start = int(sys.argv[1])
    end = int(sys.argv[2])
except:
    print sys.argv[0], '<start> <end>'
    sys.exit(1)
ssl_config = {'key_file':'certs/userCerts/userkey-zsombor.pem', 'cert_file':'certs/userCerts/usercert-zsombor.pem','ca_dir':'/etc/grid-security/certificates'}
a = storage.client.AHashClient('https://dingding.uio.no:60000/AHash', ssl_config = ssl_config)
for i in range(start, end):
    print i,
    change_start = time.time()
    a.change({i: ['0', 'set', 'entries', i, '0', {}]})
    print "%0.5f" % (time.time()-change_start),
    get_start = time.time()
    a.get(['0'])
    print "%0.5f" % (time.time()-get_start)
