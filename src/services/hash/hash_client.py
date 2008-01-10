import arc, httplib, sys
from xml.dom.minidom import parseString
ns = arc.NS({'hash':'urn:hash'})
out = arc.PayloadSOAP(ns)
if sys.argv[1] == 'get':
    ids = out.NewChild('hash:get').NewChild('hash:IDs')
    for i in sys.argv[2:]:
        ids.NewChild('hash:ID').Set(str(i))
elif sys.argv[1] == 'change':
    requests = out.NewChild('hash:change').NewChild('hash:changeRequestList')
    request = requests.NewChild('hash:changeRequestElement')
    request.NewChild('hash:changeID').Set('0')
    request.NewChild('hash:ID').Set(sys.argv[2])
    request.NewChild('hash:changeType').Set(sys.argv[3])
    request.NewChild('hash:section').Set(sys.argv[4])
    request.NewChild('hash:property').Set(sys.argv[5])
    request.NewChild('hash:value').Set(sys.argv[6])
msg = out.GetXML()
print 'Request:'
print msg
print
h = httplib.HTTPConnection('localhost', 60000)
h.request('POST', '/Hash',msg,)
r = h.getresponse()
print 'Response:', r.status, r.reason
print parseString(r.read()).toprettyxml()
