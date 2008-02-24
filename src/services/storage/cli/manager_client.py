import arc, sys, time
from storage.xmltree import XMLTree
from storage.client import ManagerClient
args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False
manager = ManagerClient('http://localhost:60000/Manager', print_xml)
if len(args) == 0 or args[0] not in ['stat', 'makeCollection']:
    print 'Supported methods: stat, makeCollection' 
else:
    command = args.pop(0)
    if command == 'stat':
        if len(args) < 1:
            print 'Usage: stat <LN> [<LN> ...]'
        else:
            request = dict([(i, args[i]) for i in range(len(args))])
            print 'stat', request
            print manager.stat(request)
    elif command == 'makeCollection':
        if len(args) < 1:
            print 'Usage: makeCollection <LN>'
        else:
            LNs = [args[0]]
            print 'makeCollection', LNs
            print manager.makeCollection(LNs)
