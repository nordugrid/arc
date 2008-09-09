import arc, sys, time, os
from storage.xmltree import XMLTree
from storage.client import LibrarianClient
args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False
try:
    librarian_url = os.environ['ARC_LIBRARIAN_URL']
    # print '- The URL of the Librarian:', librarian_url
except:
    librarian_url = 'http://localhost:60000/Librarian'
    print '- ARC_LIBRARIAN_URL environment variable not found, using', librarian_url
ssl_config = {}
if librarian_url.startswith('https'):
    try:
        ssl_config['key_file'] = os.environ['ARC_KEY_FILE']
        ssl_config['cert_file'] = os.environ['ARC_CERT_FILE']
        # print '- The key file:', ssl_config['key_file']
        # print '- The cert file:', ssl_config['cert_file']
    except:
        ssl_config = {}
        print '- ARC_KEY_FILE or ARC_CERT_FILE environment variable not found, SSL disabled'
librarian = LibrarianClient(librarian_url, print_xml, ssl_config = ssl_config)    
if len(args) == 0 or args[0] not in ['new', 'get', 'traverseLN', 'modifyMetadata', 'remove']:
    print 'Supported methods: new get traverseLN modifyMetadata'
else:
    command = args.pop(0)
    if command == 'new':
        if len(args) < 1:
            print 'Usage: new {collection,file} [<GUID>]'
        else:
            type = args.pop(0)
            metadata = {}
            metadata[('entry','type')] = type
            metadata[('timestamps','created')] = str(time.time())
            if len(args) > 0:
                metadata[('entry', 'GUID')] = args[0]
            request = {'0' : metadata}
            print 'new', request
            print librarian.new(request)
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
            print librarian.get(IDs, neededMetadata)
    elif command == 'traverseLN':
        if len(args) < 1:
            print 'Usage: traverseLN <LN> [<LN> ...]'
        else:
            request = {}
            for i in range(len(args)):
                request[str(i)] = args[i]
            print 'traverseLN', request
            print librarian.traverseLN(request)
    elif command == 'modifyMetadata':
        if len(args) < 5:
            print 'Usage: modifyMetadata <GUID> <changeType> <section> <property> <value>'
        else:
            request = {'0' : args}
            print 'modifyMetadata', request
            print librarian.modifyMetadata(request)
    elif command == 'remove':
        if len(args) < 1:
            print 'Usage: remove <GUID> [<GUID>, ...]'
        else:
            request = dict([(str(i), args[i]) for i in range(len(args))])
            print 'remove', request
            print librarian.remove(request)
