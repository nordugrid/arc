import arc, sys, os
from arcom.xmltree import XMLTree
from storage.client import AHashClient
print ":".join(sys.argv)
args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False
try:
    ahash_url = os.environ['ARC_AHASH_URL']
    # print '- The URL of the A-Hash:', ahash_url
except:
    ahash_url = 'http://localhost:60000/AHash'
    print '- ARC_AHASH_URL environment variable not found, using', ahash_url
ssl_config = {}
if ahash_url.startswith('https'):
    try:
        ssl_config['key_file'] = os.environ['ARC_KEY_FILE']
        ssl_config['cert_file'] = os.environ['ARC_CERT_FILE']
        # print '- The key file:', ssl_config['key_file']
        # print '- The cert file:', ssl_config['cert_file']
    except:
        ssl_config = {}
        print '- ARC_KEY_FILE or ARC_CERT_FILE environment variable not found, SSL disabled'
ahash = AHashClient(ahash_url, print_xml, ssl_config = ssl_config)    
if len(args) == 0 or args[0] not in ['get', 'change', 'changeIf']:
    print 'Supported methods: get, change, changeIf'
else:
    command = args.pop(0)
    if command == 'get':
        if len(args) < 1:
            print 'Usage: get <ID> [<ID> ...] neededMetadata [<section> [<property>] ...] '
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
            print ahash.get(IDs, neededMetadata)
    elif command == 'change':
        if len(args) < 5:
            print 'Usage: change <ID> <changeType> <section> <property> <value>'
            print '   changeType: set | unset | delete'
        else:
            changes = {'0' : args + [{}]}
            print 'change', changes
            print ahash.change(changes)
    elif command == 'changeIf':
        if len(args) < 9:
            print 'Usage: changeIf <ID> <changeType> <section> <property> <value> <conditionType> <section> <property> <value>'
            print '   changeType: set | unset | delete'
            print '   conditionType: is | isnot | isset | unset'
        else:
            changes = {'0' : args[0:5] + [{'0' : args[5:9]}]}
            print 'change', changes
            print ahash.change(changes)
