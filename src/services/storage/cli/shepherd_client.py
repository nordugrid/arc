import arc, sys, time, StringIO, os
from arcom.xmltree import XMLTree
from storage.client import ShepherdClient, ByteIOClient
from storage.common import create_checksum
args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False
try:
    shepherd_url = os.environ['ARC_SHEPHERD_URL']
    # print '- The URL of the Shepherd:', shepherd_url
except:
    shepherd_url = 'http://localhost:60000/Shepherd'
    print '- ARC_SHEPHERD_URL environment variable not found, using', shepherd_url
ssl_config = {}
if shepherd_url.startswith('https'):
    try:
        ssl_config['key_file'] = os.environ['ARC_KEY_FILE']
        ssl_config['cert_file'] = os.environ['ARC_CERT_FILE']
        # print '- The key file:', ssl_config['key_file']
        # print '- The cert file:', ssl_config['cert_file']
    except:
        ssl_config = {}
        print '- ARC_KEY_FILE or ARC_CERT_FILE environment variable not found, SSL disabled'
shepherd = ShepherdClient(shepherd_url, print_xml, ssl_config = ssl_config)    
if len(args) == 0 or args[0] not in ['get', 'put', 'stat', 'delete', 'reporting']:
    print 'Supported methods: get put stat delete reporting'
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
    elif command == 'delete':
        if len(args) < 1:
            print 'Usage: delete <referenceID>'
        else:
            request = {'0' : args[0]}
            print 'delete', request
            print shepherd.delete(request)
    elif command == 'reporting':
        if len(args) < 1 or args[0] not in ['on', 'off']:
            print 'Usage: reporting on|off'
        else:
            doReporting = args[0] == 'on'
            print 'toggleReport', doReporting
            print shepherd.toggleReport(doReporting)
