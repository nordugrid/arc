import arc, httplib, sys
from xmltree import XMLTree
ns = arc.NS({'hash':'urn:hash'})
out = arc.PayloadSOAP(ns)
if sys.argv[1] == '-x':
    sys.argv.pop(1)
    print_xml = True
    from xml.dom.minidom import parseString
else:
    print_xml = False
if sys.argv[1] == 'get':
    if len(sys.argv) < 3:
        print 'Usage: get <ID> [<ID>...]'
        sys.exit(-1)
    tree = XMLTree(from_tree =
        ('hash:get',
            [('hash:IDs',
                [('hash:ID', i) for i in sys.argv[2:]]
            )]
        ))
    tree.add_to_node(out)
elif sys.argv[1] == 'change':
    if len(sys.argv) < 7:
        print 'Usage: change <ID> <changeType> <section> <property> <value>'
        print '   changeType: add | remove | delete | reset'
        sys.exit(-1)
    requests = out.NewChild('hash:change').NewChild('hash:changeRequestList')
    tree = XMLTree(from_tree = 
        ('hash:changeRequestElement',
            [('hash:changeID', '0'),
            ('hash:ID', sys.argv[2]),
            ('hash:changeType', sys.argv[3]),
            ('hash:section', sys.argv[4]),
            ('hash:property', sys.argv[5]),
            ('hash:value', sys.argv[6])]
        ))
    tree.add_to_node(requests)
elif sys.argv[1] == 'changeIf':
    if len(sys.argv) < 11:
        print """Usage: changeIf <ID> <changeType> <section> <property> <value> \\
            <conditionType> <section> <property> <value>"""
        print '   changeType: add | remove | delete | reset'
        print '   conditionType: has | not | exists | empty'
        sys.exit(-1)
    requests = out.NewChild('hash:change').NewChild('hash:changeRequestList')
    tree = XMLTree(from_tree = 
        ('hash:changeRequestElement',
            [('hash:changeID', '0'),
            ('hash:ID', sys.argv[2]),
            ('hash:changeType', sys.argv[3]),
            ('hash:section', sys.argv[4]),
            ('hash:property', sys.argv[5]),
            ('hash:value', sys.argv[6]),
            ('hash:conditionList',
                [('hash:condition',
                    [('hash:conditionType',sys.argv[7]),
                    ('hash:section',sys.argv[8]),
                    ('hash:property',sys.argv[9]),
                    ('hash:value',sys.argv[10])]
                )]
            )]
        ))
    tree.add_to_node(requests)
else:
    print 'Supported methods: get, change, changeIf'
    sys.exit(-1)
msg = out.GetXML()
if print_xml:
    print 'Request:'
    print parseString(msg).toprettyxml()
    print
h = httplib.HTTPConnection('localhost', 60000)
h.request('POST', '/Hash',msg,)
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
