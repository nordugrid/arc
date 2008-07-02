import arc, sys, time, StringIO
from storage.xmltree import XMLTree
from storage.client import ShepherdClient, ByteIOClient
from storage.common import create_checksum
args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False
shepherd = ShepherdClient('http://localhost:60000/Shepherd', print_xml)
if len(args) == 0 or args[0] not in ['get', 'put', 'stat', 'reporting']:
    print 'Supported methods: get put stat reporting'
else:
    command = args.pop(0)
    if command == 'get':
        if len(args) < 1:
            print 'Usage: get <referenceID>'
        else:
            request = {'0' : [('referenceID', args[0]), ('protocol', 'byteio')]} 
            print 'get', request
            response = dict(shepherd.get(request)['0'])
            if response.has_key('error'):
                print 'ERROR', response['error']
            else:
                turl = response['TURL']
                print 'Downloading:', turl
                print 'Checksum is', response['checksum']
                print '***'
                data = ByteIOClient(turl).read()
                print data
                print '***'
                print 'actual checksum is', create_checksum(StringIO.StringIO(data), response['checksumType'])
    elif command == 'put':
        if len(args) < 1:
            print 'Usage: put <data>'
        else:
            data = ' '.join(args)
            checksum = create_checksum(StringIO.StringIO(data), 'md5')
            request = {'0' : [('size', str(len(data))), ('protocol', 'byteio'), ('checksumType', 'md5'), ('checksum', checksum)]} 
            print 'put', request
            response = dict(shepherd.put(request)['0'])
            if response.has_key('error'):
                print 'ERROR', response['error']
            else:
                print 'referenceID:', response['referenceID']
                turl = response['TURL']
                print 'Uploading:', turl
                ByteIOClient(turl).write(data)
    elif command == 'stat':
        if len(args) < 1:
            print 'Usage: stat <referenceID>'
        else:
            request = {'0' : args[0]}
            print 'stat', request
            print shepherd.stat(request)
    elif command == 'reporting':
        if len(args) < 1 or args[0] not in ['on', 'off']:
            print 'Usage: reporting on|off'
        else:
            doReporting = args[0] == 'on'
            print 'toggleReport', doReporting
            print shepherd.toggleReport(doReporting)
