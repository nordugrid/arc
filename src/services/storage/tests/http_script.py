#! /usr/bin/env python
import sys, time, httplib
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
    h = httplib.HTTPConnection('localhost', 60002)
    change_msg = '''<soap-env:Envelope xmlns:ahash="http://www.nordugrid.org/schemas/ahash" xmlns:soap-enc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <soap-env:Body><ahash:change><ahash:changeRequestList><ahash:changeRequestElement><ahash:changeID>%s</ahash:changeID><ahash:ID>0</ahash:ID>
    <ahash:changeType>set</ahash:changeType><ahash:section>entries</ahash:section><ahash:property>%s</ahash:property><ahash:value>0</ahash:value>
    <ahash:conditionList/></ahash:changeRequestElement></ahash:changeRequestList></ahash:change></soap-env:Body></soap-env:Envelope>''' % (i,i)
    h.request('POST', '/AHash', change_msg)
    r = h.getresponse()
    r.read()
    print "%0.5f" % (time.time()-change_start),
    get_start = time.time()
    h = httplib.HTTPConnection('localhost', 60002)
    get_msg = '''<soap-env:Envelope xmlns:ahash="http://www.nordugrid.org/schemas/ahash" xmlns:soap-enc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <soap-env:Body><ahash:get><ahash:neededMetadataList/><ahash:IDs><ahash:ID>0</ahash:ID></ahash:IDs></ahash:get></soap-env:Body></soap-env:Envelope>'''
    h.request('POST', '/AHash', change_msg)
    r = h.getresponse()
    r.read()
    print "%0.5f" % (time.time()-get_start)
