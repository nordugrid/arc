import arc, httplib, sys, time
from storage.xmltree import XMLTree
ns = arc.NS({'cat':'urn:scatalog'})
out = arc.PayloadSOAP(ns)
if sys.argv[1] == '-x':
    sys.argv.pop(1)
    print_xml = True
    from xml.dom.minidom import parseString
else:
    print_xml = False
if sys.argv[1] == 'newCollection':
    lines = [ 
        ('cat:line', [
            ('cat:section', 'timestamps'),
            ('cat:property', 'created'),
            ('cat:value', str(time.time()))
        ])
    ]
    if len(sys.argv) > 2:
        GUID = sys.argv[2]
        lines.append(
            ('cat:line', [
                ('cat:section', 'catalog'),
                ('cat:property', 'GUID'),
                ('cat:value', GUID)
            ])
        )
    requests = out.NewChild('cat:newCollection').NewChild('cat:newCollectionRequestList')
    tree = XMLTree(from_tree =
        ('cat:newCollectionRequestElement', [
            ('cat:requestID', '0'),
            ('cat:metadata', lines)
        ])
    )
    tree.add_to_node(requests)
elif sys.argv[1] == 'get':
    if len(sys.argv) < 3:
        print 'Usage: get <GUID> [<GUID> ...]'
        sys.exit(-1)
    tree = XMLTree(from_tree =
        ('cat:get', [
            ('cat:getRequestList', [
                ('cat:getRequestElement', [
                    ('cat:GUID', GUID) 
                ]) for GUID in sys.argv[2:]
            ])
        ])
    )
    tree.add_to_node(out)
elif sys.argv[1] == 'traverseLN':
    if len(sys.argv) < 3:
        print 'Usage: traverseLN <LN> [<LN> ...]'
        sys.exit(-1)
    tree = XMLTree(from_tree =
        ('cat:traverseLN', [
            ('cat:traverseLNRequestList', [
                ('cat:traverseLNRequestElement', [
                    ('cat:requestID', '0'),
                    ('cat:LN', LN) 
                ]) for LN in sys.argv[2:]
            ])
        ])
    )
    tree.add_to_node(out)
else:
    print 'Supported methods: newCollection get traverseLN'
    sys.exit(-1)
msg = out.GetXML()
if print_xml:
    print 'Request:'
    print parseString(msg).toprettyxml()
    print
h = httplib.HTTPConnection('localhost', 60000)
h.request('POST', '/Catalog', msg)
r = h.getresponse()
print 'Response:', r.status, r.reason
resp = r.read()
if print_xml:
    print parseString(resp).toprettyxml()
t = XMLTree(from_string=resp).get('/////')
for i in t:
    print ' = '.join(i[1][0])
    for j in i[1][1:]:
        if isinstance(j[1], str):
            print '  ', ' = '.join(j)
        else:
            for k in j[1]:
                print '  ', k[0], 
                for l in k[1]:
                    print l[1],
                print
