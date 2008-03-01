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
if len(args) == 0 or args[0] not in ['newCollection', 'get', 'traverseLN', 'modifyMetadata', 'remove']:
    print 'Supported methods: newCollection get traverseLN modifyMetadata'
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
            print 'Usage: get <GUID> [<GUID> ...] neededMetadata [<section> [<property>] ...] '
        else:
            IDs = []
            while len(args) > 0 and args[0] != 'neededMetadata':
                IDs.append(args.pop(0))
            neededMetadata = []
            if len(args) > 0:
                args.pop(0)
            while len(args) > 0:
                section = args.pop(0)
                if len(args) > 0:
                    property = args.pop(0)
                else:
                    property = ''
                neededMetadata.append((section, property))
            print 'get', IDs, neededMetadata
            print catalog.get(IDs, neededMetadata)
    elif command == 'traverseLN':
        if len(args) < 1:
            print 'Usage: traverseLN <LN> [<LN> ...]'
        else:
            request = {}
            for i in range(len(args)):
                request[str(i)] = args[i]
            print 'traverseLN', request
            print catalog.traverseLN(request)
    elif command == 'modifyMetadata':
        if len(args) < 5:
            print 'Usage: modifyMetadata <GUID> <changeType> <section> <property> <value>'
        else:
            request = {'0' : args}
            print 'modifyMetadata', request
            print catalog.modifyMetadata(request)
    elif command == 'remove':
        if len(args) < 1:
            print 'Usage: remove <GUID> [<GUID>, ...]'
        else:
            request = dict([(str(i), args[i]) for i in range(len(args))])
            print 'remove', request
            print catalog.remove(request)
