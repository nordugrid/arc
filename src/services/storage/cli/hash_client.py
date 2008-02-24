import arc, sys
from storage.xmltree import XMLTree
from storage.client import HashClient
args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False
hash = HashClient('http://localhost:60000/Hash', print_xml)
if len(args) == 0 or args[0] not in ['get', 'change', 'changeIf']:
    print 'Supported methods: get, change, changeIf'
else:
    command = args.pop(0)
    if command == 'get':
        if len(args) < 1:
            print 'Usage: get <ID> [<ID>...]'
        else:
            print 'get', args
            print hash.get(args)
    elif command == 'change':
        if len(args) < 5:
            print 'Usage: change <ID> <changeType> <section> <property> <value>'
            print '   changeType: set | unset | delete'
        else:
            changes = {'0' : args + [{}]}
            print 'change', changes
            print hash.change(changes)
    elif command == 'changeIf':
        if len(args) < 9:
            print 'Usage: changeIf <ID> <changeType> <section> <property> <value> <conditionType> <section> <property> <value>'
            print '   changeType: set | unset | delete'
            print '   conditionType: is | isnot | isset | unset'
        else:
            changes = {'0' : args[0:5] + [{'0' : args[5:9]}]}
            print 'change', changes
            print hash.change(changes)
