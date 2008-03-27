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
if len(args) == 0 or args[0] not in ['stat', 'makeCollection', 'list', 'move']:
    print 'Supported methods: stat, makeCollection, list, move' 
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
            LNs = {'0': args[0]}
            print 'makeCollection', LNs
            print manager.makeCollection(LNs)
    elif command == 'list':
        if len(args) < 1:
            print 'Usage: list <LN> [<LN> ...]'
        else:
            request = dict([(str(i), args[i]) for i in range(len(args))])
            print 'list', request
            response = manager.list(request,[('catalog','')])
            print response
            for rID, entries in response.items():
                print
                print '%s:' % request[rID]
                for name, (GUID, metadata) in entries.items():
                    print '\t%s\t<%s>' % (name, metadata.get(('catalog', 'type'),'unknown'))
    elif command == 'move':
        if len(args) < 2:
            print 'Usage: move <sourceLN> <targetLN> [preserve]'
        else:
            sourceLN = args.pop(0)
            targetLN = args.pop(0)
            preserveOriginal = False
            if len(args) > 0:
                if args[0] == 'preserve':
                    preserveOriginal = True
            request = {'0' : (sourceLN, targetLN, preserveOriginal)}
            print 'move', request
            print manager.move(request)
