import arc, httplib, sys
from xml.dom.minidom import parseString
ns = arc.NS({'hash':'urn:hash'})
out = arc.PayloadSOAP(ns)
if sys.argv[1] == 'get':
    if len(sys.argv) < 3:
        print 'Usage: get <ID> [<ID>...]'
        sys.exit(-1)
    ids = out.NewChild('hash:get').NewChild('hash:IDs')
    for i in sys.argv[2:]:
        ids.NewChild('hash:ID').Set(str(i))
elif sys.argv[1] == 'change':
    if len(sys.argv) < 7:
        print 'Usage: change <ID> <changeType> <section> <property> <value>'
        print '   changeType: add | remove | delete | reset'
        sys.exit(-1)
    requests = out.NewChild('hash:change').NewChild('hash:changeRequestList')
    request = requests.NewChild('hash:changeRequestElement')
    request.NewChild('hash:changeID').Set('0')
    request.NewChild('hash:ID').Set(sys.argv[2])
    request.NewChild('hash:changeType').Set(sys.argv[3])
    request.NewChild('hash:section').Set(sys.argv[4])
    request.NewChild('hash:property').Set(sys.argv[5])
    request.NewChild('hash:value').Set(sys.argv[6])
elif sys.argv[1] == 'changeIf':
    if len(sys.argv) < 11:
        print """Usage: changeIf <ID> <changeType> <section> <property> <value> \\
            <conditionType> <section> <property> <value>"""
        print '   changeType: add | remove | delete | reset'
        print '   conditionType: has | not | exists | empty'
        sys.exit(-1)
    requests = out.NewChild('hash:changeIf').NewChild('hash:changeIfRequestList')
    request = requests.NewChild('hash:changeIfRequestElement')
    request.NewChild('hash:changeID').Set('0')
    request.NewChild('hash:ID').Set(sys.argv[2])
    request.NewChild('hash:changeType').Set(sys.argv[3])
    request.NewChild('hash:section').Set(sys.argv[4])
    request.NewChild('hash:property').Set(sys.argv[5])
    request.NewChild('hash:value').Set(sys.argv[6])
    condition = request.NewChild('hash:conditionList').NewChild('hash:condition')
    condition.NewChild('hash:conditionType').Set(sys.argv[7])
    condition.NewChild('hash:section').Set(sys.argv[8])
    condition.NewChild('hash:property').Set(sys.argv[9])
    condition.NewChild('hash:value').Set(sys.argv[10])
else:
    print 'Supported methods: get, change, changeIf'
    sys.exit(-1)
msg = out.GetXML()
print 'Request:'
print parseString(msg).toprettyxml()
print
h = httplib.HTTPConnection('localhost', 60000)
h.request('POST', '/Hash',msg,)
r = h.getresponse()
print 'Response:', r.status, r.reason
print parseString(r.read()).toprettyxml()
