import os, traceback

# namespace URIs of the storage services
ahash_uri = 'http://www.nordugrid.org/schemas/ahash'
librarian_uri = 'http://www.nordugrid.org/schemas/librarian'
bartender_uri = 'http://www.nordugrid.org/schemas/bartender'
shepherd_uri = 'http://www.nordugrid.org/schemas/shepherd'
rbyteio_uri = 'http://schemas.ggf.org/byteio/2005/10/random-access'

# For Gateway service
gateway_uri = 'urn:storagegateway'




# URI for the simple transfer mechanism of ByteIO
byteio_simple_uri = 'http://schemas.ggf.org/byteio/2005/10/transfer-mechanisms/simple'
# True and False values used in the XML representation
true = '1'
false = '0'

# the GUID of the root collection of the global storage namespace
global_root_guid = '0'
# a special entity where the data about Shepherds are stored (SEStore)
sestore_guid = '1'

def parse_url(url):
    import urlparse
    (proto, host_port, path, _, _, _) = urlparse.urlparse(url)
    if ':' in host_port:
        host, port = host_port.split(':')
    else:
        host = host_port
        port = 80
    return proto, host, int(port), path

common_supported_protocols = ['http', 'byteio','external']
CHUNKSIZE = 2**20

def upload_to_turl(turl, protocol, fobj, size = None, ssl_config = {}):
    """docstring for upload_to_turl"""
    if protocol not in common_supported_protocols:
        raise Exception, 'Unsupported protocol'
    if protocol == 'byteio':
        from storage.client import ByteIOClient
        return ByteIOClient(turl).write(fobj)
    elif protocol == 'external':
        return 
    elif protocol == 'http':
        proto, host, port, path = parse_url(turl)
        import httplib
        if proto == 'https':
            h = httplib.HTTPSConnection(host, port, key_file = ssl_config.get('key_file', None), cert_file = ssl_config.get('cert_file', None))
        else:
            h = httplib.HTTPConnection(host, port)
        if not size:
            size = os.path.getsize(fobj.name)
        print 'Uploading %s bytes...' % size,
        h.putrequest('PUT', path)
        h.putheader('Content-Length', size)
        h.endheaders()
        chunk = fobj.read(CHUNKSIZE)
        uploaded = 0
        print '00%',
        import time, sys
        t = time.time()
        while chunk:
            sys.stdout.flush()    
            h.send(chunk)
            uploaded = uploaded + len(chunk)
            if time.time() - t > 2:
                t = time.time()
                print '\b\b\b\b%02d%%' % (uploaded*100/size),
            chunk = fobj.read(CHUNKSIZE)
        print '\b\b\b\bdata sent, waiting...',
        sys.stdout.flush()    
        r = h.getresponse()
        print 'done.'
        #resp = r.read()
        return r.status

def download_from_turl(turl, protocol, fobj, ssl_config = {}):
    """docstring for download_from_turl"""
    if protocol not in common_supported_protocols:
        raise Exception, 'Unsupported protocol'
    if protocol == 'byteio':
        from storage.client import ByteIOClient
        ByteIOClient(turl).read(file = f)
    elif protocol == 'http':
        proto, host, port, path = parse_url(turl)
        import httplib
        if proto == 'https':
            h = httplib.HTTPSConnection(host, port, key_file = ssl_config.get('key_file', None), cert_file = ssl_config.get('cert_file', None))
        else:
            h = httplib.HTTPConnection(host, port)
        h.request('GET', path)
        r = h.getresponse()
        size = int(r.getheader('Content-Length', CHUNKSIZE))
        print 'Downloading %s bytes...' % size,
        chunk = r.read(CHUNKSIZE)
        downloaded = 0
        print '00%',
        import time, sys
        t = time.time()
        while chunk:
            sys.stdout.flush()    
            fobj.write(chunk)
            downloaded = downloaded + len(chunk)
            if time.time() - t > 2:
                t = time.time()
                print '\b\b\b\b%02d%%' % (downloaded*100/size),
            chunk = r.read(CHUNKSIZE)
        print '\b\b\b\bdone.'
        return r.status

def create_checksum(file, type):
    """ Create checksum of a file.
    
    create_checksum(file, type)
    
    file is an object with a 'read' method
    type is a string indicating the type of the checksum, currently only md5 is supported
    
    Returns the checksum as a string.
    """
    if type == 'md5':
        return _md5sum(file)
    else:
        raise Exception, 'Unsupported checksum type'

import md5

def _md5sum(file):
    """ Create md5 checksum of a file.
    
    _md5sum(file)
    
    file is an object with a 'read' method
    
    Returns the checksum as a string.
    """
    m = md5.new()
    while True:
        # read the file in chunks
        t = file.read(1024)
        if len(t) == 0: break # end of file
        # put the file content into the md5 object
        m.update(t)
    # get the checksum from the md5 object
    return m.hexdigest()

def import_class_from_string(str):
    """ Import the class which name is in the string
    
    import_class_from_string(str)

    'str' is the name of the class in [package.]*module.classname format.
    """
    module = '.'.join(str.split('.')[:-1]) # cut off class name
    cl = __import__(module) # import module
    for name in str.split('.')[1:]: # start from the first package name
        # step to the next package or the module or the class
        cl = getattr(cl, name)
    return cl # return the class

def serialize_ids(ids):
    """ Serialize a list of IDs into one string.
    
    serialize_ids(ids)
    
    ids is a list of IDs, which are strings
    
    Returns one string which containes all the IDs.
    """
    return ' '.join([str(id) for id in ids])

def deserialize_ids(s):
    """ Deserialize a list of IDs from a string.
    
    deserialize_ids(s)
    
    s is a string created by the 'serialize_ids' method
    
    Returns the list of IDs (which are all strings) which were originally serialized into the string.
    """
    return s.split()

def mkuid():
    """ Create a hopefully globally unique ID.
    
    mkuid()
    
    TODO: use the uuidgen provided by the ARC HED
    """
    import random
    rnd = ''.join([ chr(random.randint(0,255)) for i in range(16) ])
    rnd = rnd[:6] + chr((ord(rnd[6]) & 0x4F) | 0x40) + rnd[7:8]  + chr((ord(rnd[8]) & 0xBF) | 0x80) + rnd[9:]
    uuid = 0L
    for i in range(16):
        uuid = (uuid << 8) + ord(rnd[i])
    rnd_str = '%032x' % uuid
    return '%s-%s-%s-%s-%s' % (rnd_str[0:8], rnd_str[8:12], rnd_str[12:16],  rnd_str[16:20], rnd_str[20:])

def parse_metadata(metadatalist_node):
    """ Return the metadata which was put into an XML representation.
    
    parse_metadata(metadatalist_node)
    
    metadatalist_node is an XMLNode which has zero or more children,
        and each child should have three children called 'section', 'property', 'value')
    
    The method get the string value from each tuple of section-property-value,
    then creates a dictionary where the (section, property) pairs are the keys,
    and the (value) is the value.
    
    Example:
    
    input:
    
        <metadataList>
            <metadata>
                <section>entry</section>
                <property>type</property>
                <value>file</value>
            </metadata>
            <metadata>
                <section>states</section>
                <property>size</property>
                <value>812314</value>
            </metadata>
        </metadataList>
    
    output:
    
        {('entry','type') : 'file', ('states', 'size') : '812314'}
    """
    # get the number of children
    metadatalist_number = metadatalist_node.Size()
    # initialize the list
    metadata = []
    # for each child
    for j in range(metadatalist_number):
        # get the child node
        metadata_node = metadatalist_node.Child(j)
        # get the 'section', 'property' and 'value' children's string value
        # and append them to the list as (('section','property'),'value')
        metadata.append((
            (str(metadata_node.Get('section')),str(metadata_node.Get('property'))),
                str(metadata_node.Get('value'))))
    # create a dictionary from the list
    return dict(metadata)

def create_metadata(metadata, prefix = ''):
    """ Create an XMLTree structure from a dictionary with metadata.
    
    create_metadata(metadata, prefix = '')
    
    metadata is a dictionary with ('section','property') as keys and 'value' as values
    prefix is a string for the namespace prefix of the XML tag, empty by default.
    
    Example:
    
    input:
    
        {('entry','type') : 'file', ('states', 'size') : '812314'}
    
    output:
    
        [('metadata', [('section', 'entry'), ('property', 'type'), ('value', 'file')]),
         ('metadata', [('section', 'states'), ('property', 'size'), ('value', '812314')])]
         
    this output can be used as an XMLTree object, and can be put into an XMLNode, which will look like this:
    
            <metadata>
                <section>entry</section>
                <property>type</property>
                <value>file</value>
            </metadata>
            <metadata>
                <section>states</section>
                <property>size</property>
                <value>812314</value>
            </metadata>
    """
    # if it is an empty dictionary, just return an empty list
    if not metadata:
        return []
    # if a prefix is specified
    if prefix:
        # for each item in the metadata list get the section, property and value and put it into XMLTree form
        # the 'metadata', 'section', 'property' and 'value' strings are prefixed with the given prefix and a ':'
        return [
            (prefix + ':metadata', [ 
                (prefix + ':section', section),
                (prefix + ':property', property),
                (prefix + ':value', value)
            ]) for ((section, property), value) in metadata.items()
        ]
    else:
        # if no prefix is specified do not put a colon before the names
        return [
            ('metadata', [ 
                ('section', section),
                ('property', property),
                ('value', value)
            ]) for ((section, property), value) in metadata.items()
        ]

def node_to_data(node, names, single = False, string = True):
    """ Get some children of an XMLNode and return them in a list in the specified order using the first one as a key.
    
    node_to_data(node, names, single = False, string = True)
    
    node is an XMLNode which has some children
    names is a list of strings, the names of the children we want to extract, the first name always will be a key
    single is a boolean indicating if we want only a single value thus do not put it in a list
    string is a boolean indicating if we want the string values of the nodes or the nodes itself
    
    Example:
    
        node:
            <changeRequest>
                <changeID>0</changeID>
                <ID>123</ID>
                <section>states</section>
                <property>neededReplicas</property>
                <value>3</value>
                <somethingElse>not interesting</somethingElse>
                <changeType>set</changeType>
            </changeRequest>
            
        names: ['changeID', 'ID', 'changeType', 'section', 'property', 'value']
        
        here changeID will be the key, and all the other names will be in a list in the specified order
        
        so it returns ('0', ['123', 'set', 'states', 'neededReplicas', '3'])
        
            ('somethingElse' is not returned)
    
    Example:
    
        node:
            <getRequest>
                <GUID>11</GUID>
                <requestID>99</requestID>
            </getRequest>
            
        names: ['requestID', 'GUID']
        single: True
        
        here requestID will be the key, and GUID is the single value which won't be in a list
        
        so it returns ('99', '11')
        
            (instead of '99', ['11'])
    """
    if string:
        # if we need the strings
        # for each name get the string data of the child with that name,
        data = [str(node.Get(name)) for name in names]
    else:
        # for each name get the child node itself
        data = [node.Get(name) for name in names]
    if single:
        # return the first item as a key and the second item as a single value
        return data[0], data[1]
    else:
        # return the first item as a key, and all the rest items as a list
        return data[0], data[1:]

def get_data_node(node):
    return node.Get('Body').Child().Child()

def get_attributes(node):
    return dict([(attribute.Name(), str(attribute)) for attribute in [node.Attribute(i) for i in range(node.AttributesSize())]])

def get_child_nodes(node):
    """ Get all the children nodes of an XMLNode
    
    get_child_nodes(node)
    
    node is the XMLNode whose children we need
    """
    # the node.Size() method returns the number of children
    return [node.Child(i) for i in range(node.Size())]

def parse_node(node, names, single = False, string = True):
    """ Call node_to_data() for each child of the given node.
    
    parse_node(node, names, single = False, string = True)
    
    node is the XMLNode whose children we want to convert
    names is a list of tag names which will be returned in the specified order
    single indicates that we need only one value beside the key, do not put it into a list
    string indicates that we need the string data of the nodes, not the nodes itself.
    
    Example:
    
        xml = XMLNode('''
            <statRequestList>
                <statRequestElement>
                    <requestID>0</requestID>
                    <LN>/</LN>
                </statRequestElement>
                <statRequestElement>
                    <requestID>1</requestID>
                    <LN>/testfile</LN>
                </statRequestElement>
            </statRequestList>
        ''')
      
    parse_node(xml, ['requestID','LN']) returns:
        
        {'0': ['/'], '1': ['/testfile']}


    parse_node(xml, ['requestID','LN'], single = True) returns:
        
        {'0': '/', '1': '/testfile'}


    parse_node(xml, ['LN','requestID'], True) returns:
        
        {'/': '0', '/testfile': '1'}


    parse_node(xml, ['requestID','LN','LN']) returns:
        
        {'0': ['/', '/'], '1': ['/testfile', '/testfile']}


    """
    return dict([
        node_to_data(n, names, single, string)
            for n in get_child_nodes(node)
    ])

def parse_to_dict(node, names):
    """ Convert the children of the node to a dictionary of dictionaries.
    
    parse_to_dict(node, names)
    
    node is the XMLNode whose children we want to convert
    names is a list of tag names, for each child only these names will be included in the dictionary
    
    Example:
    
        <statResponseList>
            <statResponseElement>
                <requestID>123</requestID>
                <referenceID>abdad</referenceID>
                <state>alive</state>
                <size>871432</size>
            </statResponseElement>
            <statResponseElement>
                <requestID>456</requestID>
                <referenceID>fefeg</referenceID>
                <state>alive</state>
                <size>945</size>
            </statResponseElement>
        </statResponseList>
        
    parse_to_dict(xml, ['requestID', 'state', 'size']) returns:

        {'123': {'size': '871432', 'state': 'alive'},
         '456': {'size': '945', 'state': 'alive'}}


    parse_to_dict(xml, ['referenceID','requestID', 'state', 'size']) returns:
    
        {'abdad': {'requestID': '123', 'size': '871432', 'state': 'alive'},
         'fefeg': {'requestID': '456', 'size': '945', 'state': 'alive'}}
    """
    return dict([(str(n.Get(names[0])), dict([(name, str(n.Get(name))) for name in names[1:]]))
        for n in get_child_nodes(node)])

def create_response(method_name, tag_names, elements, payload, single = False):
    """ Creates an XMLNode payload from a dictionary of tag names and list of values.
    
    create_response(method_name, tag_names, elements, payload, single = False)
    
    method_name is the name of the method which will be used as a prefix in the name of the 'Response' tag
    tag_names is a list of names which will be used in the specified order as tag names
    elements is a dictionary where the key will be tagged as the first tag name,
        and the value is a list whose items will be tagged in the order of the tag_names list
    payload is an XMLNode, the response will be added to that
    single indicates if there is only one value per key
    
    Example:
    
        elements = {'123': ['alive', '871432'], '456': ['alive', '945']}
        tag_names = ['requestID', 'state', 'size']
        method_name = 'stat'
        payload = arc.PayloadSOAP(arc.NS())

    after create_response(method_name, tag_names, elements, payload, single = False) payload will contain:
    
        <statResponse>
            <statResponseList>
                <statResponseElement>
                    <requestID>123</requestID>
                    <state>alive</state>
                    <size>871432</size>
                </statResponseElement>
                <statResponseElement>
                    <requestID>456</requestID>
                    <state>alive</state>
                    <size>945</size>
                </statResponseElement>
            </statResponseList>
        </statResponse>

    The method_name used to prefix the 'Response' the 'ResponseList' and 'ResponseElement' node names.
    We could say
        method_name = 'ns:stat'
        tag_names = ['ns:requestID', 'ns:state', 'ns:size']
    if we want namespace prefixes.
    """
    # first create an XMLTree, then add it to the payload XMLNode
    from storage.xmltree import XMLTree
    if single:
        # if there is only a single value for each key
        tree = XMLTree(from_tree =
            (method_name + 'ResponseList', [
                (method_name + 'ResponseElement', [
                    (tag_names[0], key),
                    (tag_names[1], value)
                ]) for key, value in elements.items()
            ])
        )
    else:
        # if there is more values for a key
        tree = XMLTree(from_tree =
            (method_name + 'ResponseList', [
                (method_name + 'ResponseElement', [
                    (tag_names[0], key) # tag the key with the first item in tag_names
                ] + [ # for each item in the values list pick the next name from tag_names
                    (tag_names[i + 1], values[i]) for i in range(len(values))
                ]) for key, values in elements.items()
            ])
        )
    # create a <method_name>Response child node in the payload
    response_node = payload.NewChild(method_name + 'Response')
    # add the XMLTree to this newly created node
    tree.add_to_node(response_node)
    # return the payload XMLNode
    return payload

def remove_trailing_slash(LN):
    """ Remove the trailing slash of a Logical Name.
    
    remove_trailing_slash(LN)
    
    This method checks if the LN ends with a '/', and if it does, cut the trailing slash, and returns the LN.
    """
    while LN.endswith('/'):
        LN = LN[:-1]
    return LN

def splitLN(LN):
    """  Split a Logical Name to a 3-tuple: GUID, dirname, basename.
    
    splitLN(LN)
    
    The part before the first slash is the GUID, even if it is empty.
    The part after the last slash is the basename, could be empty.
    Between them is the dirname, could be empty or could contain several more slashes.
    
    Examples

        splitLN('/dir') returns ('', '', 'dir')
        splitLN('guid/dir') returns ('guid', '', 'dir')
        splitLN('guid/dir/file') returns ('guid', 'dir', 'file')
        splitLN('guid/dir/dir2/file') returns ('guid', 'dir/dir2', 'file')
        splitLN('/dir/dir2/file') returns ('', 'dir/dir2', 'file')

    Trailing slash:
        splitLN('guid/dir/dir2/') returns ('guid', 'dir/dir2', '')
    
        splitLN('') returns ('', '', '')
        splitLN('/') returns ('', '', '')
        splitLN('//') returns ('', '', '')
        splitLN('///') returns ('', '/', '')
        splitLN('///') returns ('', '//', '')

        splitLN('0') returns ('0', '', '')
        splitLN('0/') returns ('0', '', '')
        splitLN('something') returns ('something', '', '')
        splitLN('something/') returns ('something', '', '')    
    """
    # split it along the slashes
    parts = LN.split('/')
    # get the first part (before the first slash)
    rootguid = parts.pop(0)
    # try to get the last part, if there are more parts
    try:
        basename = parts.pop()
    except:
        basename = ''
    # put the slashes back between the rest items
    dirname = '/'.join(parts)
    return rootguid, dirname, basename


# Example of these auth classes:
#
# In [2]: p1 = AuthPolicy()
# 
# In [3]: p1['User1'] = ['+read', '+modifyMetadata', '-delete']
# 
# In [4]: p1['User2'] = ['+modifyPolicy']
# 
# In [5]: p1
# Out[5]: {'User1': ['+read', '+modifyMetadata', '-delete'], 'User2': ['+modifyPolicy']}
# 
# In [6]: s = p1.get_policy()
# 
# In [7]: print s
# <Policy xmlns="http://www.nordugrid.org/schemas/policy-arc" CombiningAlg="Deny-Overrides">
#   <Rule Effect="Permit">
#     <Description>User2 is allowed to modifyPolicy</Description>
#     <Subjects>
#       <Subject>
#         <Attribute AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/tls/identity" Type="string">User2</Attribute>
#       </Subject>
#     </Subjects>
#     <Actions>
#       <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/method" Type="string">modifyPolicy</Action>
#     </Actions>
#   </Rule>
#   <Rule Effect="Permit">
#     <Description>User1 is allowed to read</Description>
#     <Subjects>
#       <Subject>
#         <Attribute AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/tls/identity" Type="string">User1</Attribute>
#       </Subject>
#     </Subjects>
#     <Actions>
#       <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/method" Type="string">read</Action>
#     </Actions>
#   </Rule>
#   <Rule Effect="Permit">
#     <Description>User1 is allowed to modifyMetadata</Description>
#     <Subjects>
#       <Subject>
#         <Attribute AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/tls/identity" Type="string">User1</Attribute>
#       </Subject>
#     </Subjects>
#     <Actions>
#       <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/method" Type="string">modifyMetadata</Action>
#     </Actions>
#   </Rule>
#   <Rule Effect="Deny">
#     <Description>User1 is not allowed to delete</Description>
#     <Subjects>
#       <Subject>
#         <Attribute AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/tls/identity" Type="string">User1</Attribute>
#       </Subject>
#     </Subjects>
#     <Actions>
#       <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/method" Type="string">delete</Action>
#     </Actions>
#   </Rule>
# </Policy>
# 
# 
# In [9]: p2 = parse_arc_policy(s)
# 
# In [10]: p2
# Out[10]: {'User1': ['-delete', '+modifyMetadata', '+read'], 'User2': ['+modifyPolicy']}



storage_actions = ['read', 'addEntry', 'removeEntry', 'delete', 'modifyPolicy', 'modifyStates', 'modifyMetadata']
identity_type = 'http://www.nordugrid.org/schemas/policy-arc/types/tls/identity'
action_type = 'http://www.nordugrid.org/schemas/policy-arc/types/storage/action'
request_ns = 'http://www.nordugrid.org/schemas/request-arc'
all_user = 'ALL'

class AuthRequest:
    
    def __init__(self, message):
        auth = message.Auth()
        import arc
        xml = auth.Export(arc.SecAttr.ARCAuth)
        subject = xml.Get('RequestItem').Get('Subject')
        try:
            self.identity = str(subject.XPathLookup('//ra:SubjectAttribute[@AttributeId="%s"]' % identity_type, arc.NS({'ra':request_ns}))[0])
        except:
            # if there is no identity in the auth object (e.g. if not using TLS)
            self.identity = 'ANONYMOUS'
            identity_node = subject.NewChild('ra:SubjectAttribute',arc.NS({'ra':request_ns}))
            identity_node.Set(self.identity)
            identity_node.NewAttribute('AttributeId').Set(identity_type)
            identity_node.NewAttribute('Type').Set('string')
        self.subject = subject.GetXML()
    
    def get_request(self, action, format = 'ARCAuth'):
        if format not in ['ARCAuth']:
            raise Exception, 'Unsupported format %s' % format
        if format == 'ARCAuth':
            return '<Request xmlns="%s">\n  <RequestItem>\n%s\n%s  </RequestItem>\n</Request>' % \
                (request_ns, self.subject, '    <Action AttributeId="%s" Type="string">%s</Action>\n' % (action_type, action))
            
    def get_identity(self):
        return self.identity
            
    def __str__(self): 
        return self.subject
    

            
class AuthPolicy(dict):

    def get_policy(self, format  = 'ARCAuth'):
        if format not in ['ARCAuth', 'StorageAuth']:
            raise Exception, 'Unsupported format %s' % format
        if format == 'ARCAuth':
            result = []
            for identity, actions in self.items():
                if identity == all_user:
                    subjects = ''
                else:
                    subjects = ('    <Subjects>\n' +
                                '      <Subject>\n' + 
                                '        <Attribute AttributeId="%s" Type="string">%s</Attribute>\n' % (identity_type, identity) +
                                '      </Subject>\n' +
                                '    </Subjects>\n')
                raw_actions = [a for a in actions if a[1:] in storage_actions]
                actions = {}
                actions[True] = [action[1:] for action in raw_actions if action[0] == '+']
                actions[False] = [action[1:] for action in raw_actions if action[0] != '+']
                for permit, action_list in actions.items():
                    if action_list:
                        result.append('  <Rule Effect="%s">\n' % (permit and 'Permit' or 'Deny') +
                        '    <Description>%s is %s to %s</Description>\n' % (identity, permit and 'allowed' or 'not allowed', ', '.join(action_list)) +
                        subjects +
                        '    <Actions>\n' + 
                        ''.join(['      <Action AttributeId="%s" Type="string">%s</Action>\n' % (action_type, action) for action in action_list]) +
                        '    </Actions>\n' +
                        '  </Rule>\n')
            return '<Policy xmlns="http://www.nordugrid.org/schemas/policy-arc" CombiningAlg="Deny-Overrides">\n%s</Policy>\n' % ''.join(result)            
        if format == 'StorageAuth':
            return [(identity, ' '.join([a for a in actions if a[1:] in storage_actions])) for identity, actions in self.items()]
    
    def set_policy(self, policy, format = None):
        if not format:
            if isinstance(policy, list):
                format = 'StorageAuth'
            else:
                format = 'ARCAuth'
        self.clear()
        if format not in ['ARCAuth', 'StorageAuth']:
            raise Exception, 'Unsupported format %s' % format
        if format == 'ARCAuth':
            for rule in get_child_nodes(policy):
                permit = str(rule.Attribute('Effect')) == 'Permit'
                identities = []
                for subject in get_child_nodes(rule.Get('Subjects')):
                    for attribute in get_child_nodes(subject):
                        if get_attributes(attribute).get('AttributeId', '') == identity_type:
                            identities.append(str(attribute))
                actions = [(permit and '+' or '-') + str(action) \
                    for action in get_child_nodes(rule.Get('Actions')) \
                        if get_attributes(action).get('AttributeId', '') == action_type]
                for identity in identities:
                    self[identity] = actions + self.get(identity, [])
        if format == 'StorageAuth':
            for identity, actionstring in policy:
                self[identity] = actionstring.split()

def parse_arc_policy(policy):
    import arc
    p = AuthPolicy()
    p.set_policy(arc.XMLNode(policy))
    return p
    
def parse_storage_policy(metadata):
    import arc
    p = AuthPolicy()
    p.set_policy([(property, value) for (section, property), value in metadata.items() if section == 'policy'])
    if metadata.has_key(('entry','owner')):
        p[metadata[('entry','owner')]] = ['+' + action for action in storage_actions]        
    return p

def make_decision_metadata(metadata, request):
    import arc
    #return arc.DECISION_PERMIT
    policy = parse_storage_policy(metadata).get_policy()
    #print 'DECISION NEEDED\nPOLICY:\n%s\nREQUEST:\n%s\n' % (policy, request)
    try:
        decision = make_decision(policy, request)
    except:
        print 'DECISION ERROR. PERMITTING.'
        decision = arc.DECISION_PERMIT            
    #if decision == arc.DECISION_PERMIT:
    #    print 'PERMITTED!'
    #else:
    #    print 'DENIED! (%s)' % decision
    return decision

def make_decision(policy, request):
    import arc
    loader = arc.EvaluatorLoader()
    evaluator = loader.getEvaluator('arc.evaluator')
    p = loader.getPolicy('arc.policy', arc.Source(str(policy)))
    evaluator.addPolicy(p)
    r = loader.getRequest('arc.request', arc.Source(str(request)))
    response = evaluator.evaluate(r)
    responses = response.getResponseItems()
    response_list = [responses.getItem(i).res for i in range(responses.size())]
    print 'RESPONSE_LIST = ', response_list
    return response_list[0]
    # if response_list.count(arc.DECISION_DENY) > 0:
    #     return 'deny'
    # if response_list.count(arc.DECISION_PERMIT) > 0:
    #     return 'permit'
    # if response_list.count(arc.DECISION_NOT_APPLICABLE) > 0:
    #     return 'not_applicable'
    # return 'indeterminate'

def parse_ssl_config(cfg):
    try:
        client_ssl_node = cfg.Get(('ClientSSLConfig'))
        if client_ssl_node.Size() == 0:
            return {}
        ssl_config = {}
        ssl_config['key_file'] = str(client_ssl_node.Get('KeyPath'))
        ssl_config['cert_file'] = str(client_ssl_node.Get('CertificatePath'))
        ssl_config['ca_file'] = str(client_ssl_node.Get('CACertificatePath'))
        return ssl_config
    except:
        traceback.print_exc()
        return {}
