import arc, sys, time
from storage.xmltree import XMLTree
from storage.client import CatalogClient
args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False
catalog = CatalogClient('http://localhost:60000/Catalog', print_xml)
if len(args) == 0 or args[0] not in ['newCollection', 'get', 'traverseLN']:
    print 'Supported methods: newCollection get traverseLN'
else:
    command = args.pop(0)
    if command == 'newCollection':
        metadata = {}
        metadata[('timestamps','created')] = str(time.time())
        if len(args) > 0:
            metadata[('catalog', 'GUID')] = args[0]
        request = {'0' : metadata}
        print 'newCollection', request
        print catalog.newCollection(request)
    elif command == 'get':
        if len(args) < 1:
            print 'Usage: get <GUID> [<GUID> ...]'
        else:
            print 'get', args
            print catalog.get(args)
    elif command == 'traverseLN':
        if len(args) < 1:
            print 'Usage: traverseLN <LN> [<LN> ...]'
        else:
            request = {}
            for i in range(len(args)):
                request[str(i)] = args[i]
            print 'traverseLN', request
            print catalog.traverseLN(request)
