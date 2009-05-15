#! /usr/bin/env python
import storage.client
ssl_config = {'key_file':'certs/userCerts/userkey-zsombor.pem', 'cert_file':'certs/userCerts/usercert-zsombor.pem','ca_dir':'/etc/grid-security/certificates'}
#a = storage.client.AHashClient('https://localhost:60002/AHash', ssl_config = ssl_config)
a = storage.client.AHashClient('https://dingding.uio.no:60000/AHash', ssl_config = ssl_config)
print a.get(['0'])
