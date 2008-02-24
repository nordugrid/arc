import arc, httplib, sys, time
from storage.xmltree import XMLTree
ns = arc.NS({'man':'urn:smanager'})
out = arc.PayloadSOAP(ns)
if sys.argv[1] == '-x':
    sys.argv.pop(1)
    print_xml = True
    from xml.dom.minidom import parseString
else:
    print_xml = False
if sys.argv[1] == 'stat':
    if len(sys.argv) < 3:
        print 'Usage: stat <LN> [<LN> ...]'
        sys.exit(-1)
    LNs = sys.argv[2:]
    tree = XMLTree(from_tree =
        ('man:stat', [
            ('man:statRequestList', [
                ('man:statRequestElement', [
                    ('man:requestID', rID),
                    ('man:LN', LNs[rID]) 
                ]) for rID in range(len(LNs))
            ])
        ])
    )
    tree.add_to_node(out)
elif sys.argv[1] == 'makeCollection':
    if len(sys.argv) < 3:
        print 'Usage: makeCollection <LN>'
        sys.exit(-1)
    LN = sys.argv[2]
    tree = XMLTree(from_tree =
        ('man:makeCollection', [
            ('man:makeCollectionRequestList', [
                ('man:makeCollectionRequestElement', [
                    ('man:requestID', 0),
                    ('man:LN', LN),
                    ('man:metadataList', [
                        ('man:metadata', [
                            ('man:section', 'states'),
                            ('man:property', 'closed'),
                            ('man:value', '0')
                        ])
                    ])
                ])
            ])
        ])
    )
    tree.add_to_node(out)
else:
    print 'Supported methods: stat, makeCollection'
    sys.exit(-1)
msg = out.GetXML()
if print_xml:
    print 'Request:'
    print parseString(msg).toprettyxml()
    print
h = httplib.HTTPConnection('localhost', 60000)
h.request('POST', '/Manager', msg)
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
