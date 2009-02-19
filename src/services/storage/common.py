import os, traceback

# the GUID of the root collection of the global storage namespace
global_root_guid = '0'
# a special entity where the data about Shepherds are stored (SEStore)
sestore_guid = '1'
# a special entity where the list of A-Hashes is stored
ahash_list_guid = '2'

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
        from arcom import parse_url
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
        from arcom import parse_url
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

from arcom.security import storage_actions, AuthPolicy, make_decision

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
    print 'DECISION NEEDED\nPOLICY:\n%s\nREQUEST:\n%s\n' % (policy, request)
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