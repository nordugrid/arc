#!/usr/bin/env python

import sys, time, os
import xml.dom.minidom

# This part is copied from common.py

manager_uri = 'urn:storagemanager'
rbyteio_uri = 'http://schemas.ggf.org/byteio/2005/10/random-access'
# URI for the simple transfer mechanism of ByteIO
byteio_simple_uri = 'http://schemas.ggf.org/byteio/2005/10/transfer-mechanisms/simple'
# True and False values used in the XML representation
true = '1'
false = '0'

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

def get_child_nodes(node):
    """ Get all the children nodes of an XMLNode
    
    get_child_nodes(node)
    
    node is the XMLNode whose children we need
    """
    # the node.Size() method returns the number of children
    return [node.Child(i) for i in range(node.Size())]

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



# This part is copied from fakexmlnode.py

class FakeXMLNode(object):
    def __init__(self, from_string = '', from_node = None, doc_node = None):
        if from_node:
            self.node = from_node
            self.doc = doc_node
        else:
            self.doc = xml.dom.minidom.parseString(from_string)
            self.node = self.doc.childNodes[0]
            
    def __str__(self):
        """docstring for __str__"""
        text = ''.join([child.nodeValue for child in self.node.childNodes if child.nodeType == child.TEXT_NODE])
        return text
    
    def _get_real_children(self):
        """docstring for _get_real_children"""
        return [child for child in self.node.childNodes if child.nodeType != child.TEXT_NODE]
    
    def Size(self):
        """docstring for Size"""
        return len(self._get_real_children())
    
    def Child(self, no = 0):
        """docstring for Child"""
        return FakeXMLNode(from_node = self._get_real_children()[no], doc_node = self.doc)
    
    def FullName(self):
        """docstring for FullName"""
        return self.node.nodeName
        
    def Name(self):
        """docstring for Name"""
        return self.node.localName

    def Prefix(self):
        """docstring for Prefix"""
        return self.node.prefix
        
    def GetXML(self):
        """docstring for GetXML"""
        return self.node.toxml()
        
    def Get(self, tagname):
        """docstring for Get"""
        return [FakeXMLNode(from_node = child, doc_node = self.doc) for child in self.node.childNodes if child.nodeName == tagname or child.localName == tagname][0]
        
    def NewChild(self, tagname):
        """docstring for NewChild"""
        return FakeXMLNode(from_node = self.node.appendChild(self.doc.createElement(tagname)), doc_node = self.doc)
    
    def Set(self, text_data):
        """docstring for Set"""
        self.node.appendChild(self.doc.createTextNode(text_data))            
    
# This part is copied form xmltree.py and the 'xmlnode_class = arc.XMLNode' is changed

class XMLTree:
    def __init__(self, from_node = None, from_string = '', from_tree = None, rewrite = {}, forget_namespace = False, xmlnode_class = None):
        """ Constructor of the XMLTree class

        XMLTree(from_node = None, from_string = '', from_tree = None, rewrite = {}, forget_namespace = False)

        'from_tree' could be tree structure or an XMLTree object
        'from_string' could be an XML string
        'from_node' could be an XMLNode
        'rewrite' is a dictionary, if an XML node has a name which is a key in this dictionary,
            then it will be renamed as the value of that key
        'forget_namespace' is a boolean, if it is true, the XMLTree will not contain the namespace prefixes

        'from_tree' has the highest priority, if it is not None,
            then the other two is ignored.
        If 'from_tree' is None but from_string is given, then from_node is ignored.
        If only 'from_node' is given, then it will be the choosen one.
        In this case you may simply use:
            tree = XMLTree(node)
        """
        if from_tree:
            # if a tree structure is given, set the internal variable with it
            # if this is an XMLTree object, get just the data from it
            if isinstance(from_tree,XMLTree):
                self._data = from_tree._data
            else: 
                self._data = from_tree
        else:
            if from_node:
                # if no from_tree is given, and we have an XMLNode, just save it
                x = from_node
            else:
                # if no from_tree and from_node is given, try to parse the string
                if not xmlnode_class:
                    xmlnode_class = FakeXMLNode
                x = xmlnode_class(from_string)
            # set the internal tree structure to (<name of the root node>, <rest of the document>)
            # where <rest of the document> is a list of the child nodes of the root node
            self._data = (self._getname(x, rewrite, forget_namespace), self._dump(x, rewrite, forget_namespace))

    def _getname(self, node, rewrite = {}, forget_namespace = False):
        # gets the name of an XMLNode, with namespace prefix if it has one
        if not forget_namespace and node.Prefix():
            name = node.FullName()
        else: # and without namespace prefix if it has no prefix
            name = node.Name()
        return rewrite.get(name,name)

    def _dump(self, node, rewrite = {}, forget_namespace = False):
        # recursive method for converting an XMLNode to XMLTree structure
        size = node.Size() # get the number of children of the node
        if size == 0: # if it has no child, get the string
            return str(node)
        children = [] # collect the children
        for i in range(size):
            children.append(node.Child(i))
        # call itself recursively for each children
        return [(self._getname(n, rewrite, forget_namespace), self._dump(n, rewrite, forget_namespace)) for n in children ]

    def add_to_node(self, node, path = None):
        """ Adding a tree structure to an XMLNode.

        add_to_node(node, path = None)
        
        'node' is the XMLNode we want to add to
        'path' selects the part of the XMLTree we want to add
        """
        # selects the part we want
        data = self.get(path)
        # call the recursive helping method
        self._add_to_node(data, node)

    def _add_to_node(self, data, node):
        # recursively add the tree structure to the node
        for element in data:
            # we want to avoid empty tags in XML
            if element[0]:
                # for each child in the tree create a child in the XMLNode
                child_node = node.NewChild(element[0])
                # if the node has children:
                if isinstance(element[1],list):
                    self._add_to_node(element[1], child_node)
                else: # if it has no child, create a string from it
                    child_node.Set(str(element[1]))

    def pretty_xml(self, indent = ' ', path = None, prefix = ''):
        data = self.get(path)
        return self._pretty_xml(data, indent, level = 0, prefix = prefix)

    def _pretty_xml(self, data, indent, level, prefix ):
        out = []
        for element in data:
            if element[0]:
                if isinstance(element[1], list):
                    out.append(
                        prefix + indent * level + '<%s>\n' % element[0] +
                            self._pretty_xml(element[1], indent, level+1, prefix) + '\n' +
                        prefix + indent * level +'</%s>' % element[0]
                    )
                else:
                    out.append(prefix + indent * level + '<%s>%s</%s>' % (element[0], element[1], element[0]))
        return '\n'.join(out)
            

    def __str__(self):
        return str(self._data)

    def _traverse(self, path, data):
        # helping function for recursively traverse the tree
        # 'path' is a list of the node names, e.g. ['root','key1']
        # 'data' is the data of a tree-node,
        # e.g. ('root', [('key1', 'value'), ('key2', 'value')])
        # if the first element of the path and the name of the node is equal
        #   or if the element of the path is empty, it matches all node names
        # if not, then we have no match here, return an empty list
        if path[0] != data[0] and path[0] != '':
            return []
        # if there are no more path-elements, then we are done
        # we've just found what we looking for
        if len(path) == 1:
            return [data]
        # if there are more path-elements, but this is a string node
        # then no luck, we cannot proceed, return an empty list
        if isinstance(data[1],str):
            return []
        # if there are more path-elements, and this node has children
        ret = []
        for d in data[1]:
            # let's recurively ask all child if they have any matches
            # and collect the matches
            ret.extend( self._traverse(path[1:], d) )
        # return the matches
        return ret

    def get(self, path = None):
        """ Returns the parts of the XMLTree which match the path.

        get(path = None)

        if 'path' is not given, it defaults to the root node
        """
        if path: # if path is given
            # if it is not starts with a slash
            if not path.startswith('/'):
                raise Exception, 'invalid path (%s)' % path
            # remove the starting slash
            path = path[1:]
            # split the path to a list of strings
            path = path.split('/')
        else: # if path is not given
            # set it to the root node
            path = [self._data[0]]
        # gets the parts which are selected by this path
        return self._traverse(path, self._data)

    def get_trees(self, path = None):
        """ Returns XMLTree object for each subtree which match the path.

        get_tress(path = None)
        """
        # get the parts match the path and convert them to XMLTree
        return [XMLTree(from_tree = t) for t in self.get(path)]

    def get_value(self, path = None, *args):
        """ Returns the value of the selected part.

        get_value(path = None, [default])

        Returns the value of the node first matched the path.
        This is one level deeper than the value returned by the 'get' method.
        If there is no such node, and a default is given,
        it will return the default.
        """
        try:
            # use the get method then get the value of the first result
            return self.get(path)[0][1]
        except:
            # there was an error
            if args: # if any more argumentum is given
                # the first will be the default
                return args[0]
            raise

    def add_tree(self, tree, path = None):
        """ Add a new subtree to a path.

        add_tree(tree, path = None)
        """
        # if this is a real XMLTree object, get just the data from it
        if isinstance(tree,XMLTree):
            tree = tree._data
        # get the first node selected by the path and append the new subtree to it
        self.get(path)[0][1].append(tree)
    
    def get_values(self, path = None):
        """ Get all the values selected by a path.

        get_values(path = None)

        Like get_value but gets all values not just the first
        This has no default value.
        """
        try:
            # get just the value of each node
            return [d[1] for d in self.get(path)]
        except:
            return []

    def _dict(self, value, keys):
        # helper method for changing keys
        if keys:
            # if keys is given use only the keys which is in it
            # and translete them to new keys (the values of the 'keys' dictionary)
            return dict([(keys[k],v) for (k,v) in value if k in keys.keys()])
        else: # if keys is empty, use all the data
            return dict(value)
    
    def get_dict(self, path = None, keys = {}):
        """ Returns a dictionary from the first node the path matches.

        get_dict(path, keys = {})

        'keys' is a dictionary which filters and translate the keys
            e.g. if keys is {'hash:line':'line'}, it will only return
            the 'hash:line' nodes, and will call them 'line'
        """
        return self._dict(self.get_value(path,[]),keys)

    def get_dicts(self, path = None, keys = {}):
        """ Returns a list of dictionaries from all the nodes the path matches.

        get_dicts(path, keys = {})

        'keys' is a dictionary which filters and translate the keys
            e.g. if keys is {'hash:line':'line'}, it will only return
            the 'hash:line' nodes, and will call them 'line'
        """
        return [self._dict(v,keys) for v in self.get_values(path)]

import base64

# This part is only in this file

class FakeNS(object):
    """docstring for FakeNS"""
    def __init__(self, NS):
        self.NS = NS
        
    def getNamespaces(self):
        """docstring for getNamespaces"""
        return ''.join(['xmlns:%s="%s"' % (prefix, urn) for prefix, urn in self.NS.items()])         

# This part is from the client.py file, but is changed

class Client:
    """ Base Client class for sending SOAP messages to services """

    NS_class = FakeNS

    def __init__(self, url, ns, print_xml = False):
        """ The constructor of the Client class.
        
        Client(url, ns, print_xml = false)
        
        url is the URL of the service
        ns contains the namespaces we want to use with each message
        print_xml is for debugging, prints all the SOAP messages to the screen
        """
        import urlparse
        (_, host_port, path, _, _, _) = urlparse.urlparse(url)
        if ':' in host_port:
            host, port = host_port.split(':')
        else:
            host = host_port
            port = 80
        self.host = host
        self.port = port
        self.path = path
        self.ns = ns
        self.url = url
        self.print_xml = print_xml
        self.xmlnode_class = FakeXMLNode

    def create_soap_envelope(self, ns):
        """docstring for create_soap_envelope"""
        namespaces = ns.getNamespaces()
        xml_text = '<soap-env:Envelope %s xmlns:soap-enc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soap-env:Body/></soap-env:Envelope>' % namespaces
        return FakeXMLNode(from_string = xml_text)

    def call(self, tree, return_tree_only = False):
        """ Create a SOAP message from an XMLTree and send it to the service.
        
        call(tree, return_tree_only = False)
        
        tree is an XMLTree object containing the content of the request
        return_tree_only indicates that we only need to put the response into an XMLTree
        """
        # create a new PayloadSOAP object with the given namespace
        out = self.create_soap_envelope(self.ns)
        # add the content of the XMLTree to the XMLNode of the SOAP object
        tree.add_to_node(out.Child())
        if self.print_xml:
            msg = out.GetXML()
            print 'Request:'
            print XMLTree(out).pretty_xml(indent = '    ', prefix = '')
            print
        # call the service and get back the response, and the HTTP status
        resp, status, reason = self.call_raw(out)
        if self.print_xml:
            print 'Response:'
            try:
                print XMLTree(from_string = resp).pretty_xml(indent = '    ', prefix = '')
            except:
                print resp
            print
        if return_tree_only:
            # wrap the response into an XMLTree and return only the tree
            return XMLTree(from_string = resp, forget_namespace = True).get_trees('///')[0]
        else:
            return resp, status, reason

    def call_raw(self, outpayload):
        """ Send a POST request with the SOAP XML message.
        
        call_raw(outpayload)
        
        outpayload is an XMLNode with the SOAP message
        """
        # TODO: use the client-side libs of ARCHED
        import httplib
        # create an HTTP connection
        h = httplib.HTTPConnection(self.host, self.port)
        # get the XML from outpayload, and send it as POST
        h.request('POST', self.path, outpayload.GetXML())
        # get the response object
        r = h.getresponse()
        # read the response data
        resp = r.read()
        # return the data and the status
        return resp, r.status, r.reason

# This part is copied from client.py

class ManagerClient(Client):
    """ Client for the Storage Manager service. """
    
    def __init__(self, url, print_xml = False):
        """ Constructior of the client.
        
        ManagerClient(url, print_xml = False)
        
        url is the URL of the Manager service
        if print_xml is true this will print the SOAP messages
        """
        # sets the namespace
        ns = self.NS_class({'man':manager_uri})
        # calls the superclass' constructor
        Client.__init__(self, url, ns, print_xml)

    def stat(self, requests):
        """ Get metadata of a file or collection
        
        stat(requests)
        
        requests is a dictionary where requestIDs are the keys, and Logical Names are the values.
        this method returns a dictionary for each requestID which contains all the metadata with (section, dictionary) as the key
        
        e.g.
        
        In: {'frodo':'/', 'sam':'/testfile'}
        Out: 
            {'frodo': {('entry', 'type'): 'collection',
                       ('entries', 'testdir'): '4cabc8cb-599d-488c-a253-165f71d4e180',
                       ('entries', 'testfile'): 'cf05727b-73f3-4318-8454-16eaf10f302c',
                       ('states', 'closed'): '0'},
             'sam': {('entry', 'type'): 'file',
                     ('locations', 'http://localhost:60000/Element 00d6388c-42df-441c-8d9f-bd78c2c51667'): 'alive',
                     ('locations', 'http://localhost:60000/Element 51c9b49a-1472-4389-90d2-0f18b960fe29'): 'alive',
                     ('states', 'checksum'): '0927c28a393e8834aa7b838ad8a69400',
                     ('states', 'checksumType'): 'md5',
                     ('states', 'neededReplicas'): '5',
                     ('states', 'size'): '11'}}
        """
        tree = XMLTree(from_tree =
            ('man:stat', [
                ('man:statRequestList', [
                    ('man:statRequestElement', [
                        ('man:requestID', requestID),
                        ('man:LN', LN) 
                    ]) for requestID, LN in requests.items()
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = self.xmlnode_class(msg)
        elements = parse_node(xml.Child().Child().Child(),
            ['requestID', 'metadataList'], single = True, string = False)
        return dict([(str(requestID), parse_metadata(metadataList))
            for requestID, metadataList in elements.items()])

    def getFile(self, requests):
        """ Initiate download of a file.
        
        getFile(requests)
        
        requests is a dicitonary with requestID as key and (LN, protocols) as value,
            where LN is the Logical Name
            protocols is a list of strings: the supported transfer protocols by the client
        returns a dictionary with requestID as key and [success, TURL, protocol] as value, where
            success is the status of the request
            TURL is the Transfer URL
            protocol is the name of the choosen protocol
            
        Example:
        In: {'1':['/', ['byteio']], 'a':['/testfile', ['ftp', 'byteio']]}
        Out: 
            {'1': ['is not a file', '', ''],
             'a': ['done',
                   'http://localhost:60000/byteio/29563f36-e9cb-47eb-8186-0d720adcbfca',
                   'byteio']}
        """
        tree = XMLTree(from_tree =
            ('man:getFile', [
                ('man:getFileRequestList', [
                    ('man:getFileRequestElement', [
                        ('man:requestID', rID),
                        ('man:LN', LN)
                    ] + [
                        ('man:protocol', protocol) for protocol in protocols
                    ]) for rID, (LN, protocols) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(xml.Child().Child().Child(), ['requestID', 'success', 'TURL', 'protocol'])
    
    def putFile(self, requests):
        """ Initiate uploading a file.
        
        putFile(requests)
        
        requests is a dictionary with requestID as key, and (LN, metadata, protocols) as value, where
            LN is the Logical Name
            metadata is a dictionary with (section,property) as key, it contains the metadata of the new file
            protocols is a list of protocols supported by the client
        returns a dictionary with requestID as key, and (success, TURL, protocol) as value, where
            success is the state of the request
            TURL is the transfer URL
            protocol is the name of the choosen protocol
            
        Example:
        In: {'qwe': ['/newfile',
                    {('states', 'size') : 1055, ('states', 'checksum') : 'none', ('states', 'checksumType') : 'none'},
                    ['byteio']]}
        Out: 
            {'qwe': ['done',
                     'http://localhost:60000/byteio/d42f0993-79a8-4bba-bd86-84324367c65f',
                     'byteio']}
        """
        tree = XMLTree(from_tree =
            ('man:putFile', [
                ('man:putFileRequestList', [
                    ('man:putFileRequestElement', [
                        ('man:requestID', rID),
                        ('man:LN', LN),
                        ('man:metadataList', create_metadata(metadata, 'man')),
                    ] + [
                        ('man:protocol', protocol) for protocol in protocols
                    ]) for rID, (LN, metadata, protocols) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(xml.Child().Child().Child(), ['requestID', 'success', 'TURL', 'protocol'])

    def delFile(self, requests):
        """ Initiate deleting a file.
        
        delFile(requests)
        
        requests is a dictionary with requestID as key, and (LN) as value, where
        LN is the Logical Name of the file to be deleted
        
        returns a dictionary with requestID as key, and (success) as value,
        where success is the state of the request 
        (one of 'deleted', 'noSuchLN', 'denied)

        Example:
        In: {'fish': ['/cod']}
        Out: {'fish': ['deleted']}
        """
        tree = XMLTree(from_tree =
            ('man:delFile', [
                ('man:delFileRequestList', [
                    ('man:delFileRequestElement', [
                        ('man:requestID', requestID),
                        ('man:LN', LN) 
                    ]) for requestID, LN in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(xml.Child().Child().Child(), ['requestID', 'success'], single = True)


    def addReplica(self, requests, protocols):
        """ Add a new replica to an existing file.
        
        addReplica(requests, protocols)
        
        requests is a dictionary with requestID as key and GUID as value.
        protocols is a list of protocols supported by the client
        
        returns a dictionary with requestID as key and (success, TURL, protocol) as value, where
            success is the status of the request,
            TURL is the transfer URL
            protocol is the choosen protocol
            
        Example:
        
        In: requests = {'001':'c9c82371-4773-41e4-aef3-caf7c7eaf6f8'}, protocols = ['http','byteio']
        Out:
            {'001': ['done',
                     'http://localhost:60000/byteio/c94a77a1-347c-430c-ae1b-02d83786fb2d',
                    'byteio']}
        """
        tree = XMLTree(from_tree =
            ('man:addReplica', [
                ('man:addReplicaRequestList', [
                    ('man:putReplicaRequestElement', [
                        ('man:requestID', rID),
                        ('man:GUID', GUID),
                    ]) for rID, GUID in requests.items()
                ])
            ] + [
                ('man:protocol', protocol) for protocol in protocols
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(xml.Child().Child().Child(), ['requestID', 'success', 'TURL', 'protocol'])

    def makeCollection(self, requests):
        """ Create a new collection.
        
        makeCollection(requests)

        requests is a dictionary with requestID as key and (LN, metadata) as value, where
            LN is the Logical Name
            metadata is the metadata of the new collection, a dictionary with (section, property) as key
        returns a dictionary with requestID as key and the state of the request as value
        
        In: {'b5':['/coloredcollection', {('metadata','color') : 'light blue'}]}
        Out: {'b5': 'done'}
        """
        tree = XMLTree(from_tree =
            ('man:makeCollection', [
                ('man:makeCollectionRequestList', [
                    ('man:makeCollectionRequestElement', [
                        ('man:requestID', rID),
                        ('man:LN', LN),
                        ('man:metadataList', create_metadata(metadata, 'man'))
                    ]) for rID, (LN, metadata) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(xml.Child().Child().Child(), ['requestID', 'success'], single = True)

    def list(self, requests, neededMetadata = []):
        """ List the contents of a collection.
        
        list(requests, neededMetadata = [])
        
        requests is a dictionary with requestID as key and Logical Name as value
        neededMetadata is a list of (section, property) pairs
            if neededMetadata is empty, list will return all metadata for each collection-entry
            otherwise just those values will be returnd which has a listed (section, property)
            if a property is empty means that all properties will be listed from that section
        returns a dictionary with requestID as key and (entries, status) as value, where
            entries is a dictionary with the entry name as key and (GUID, metadata) as value
            status is the status of the request
        
        Example:
        In: requests = {'jim': '/', 'kirk' : '/testfile'}, neededMetadata = [('states','size'),('entry','type')]
        Out: 
            {'jim': ({'coloredcollection': ('cab8d235-4afa-4e33-a85f-fde47b0240d1', {('entry', 'type'): 'collection'}),
                      'newdir': ('3b200a34-0d63-4d15-9b01-3693685928bc', {('entry', 'type'): 'collection'}),
                      'newfile': ('c9c82371-4773-41e4-aef3-caf7c7eaf6f8', {('entry', 'type'): 'file', ('states', 'size'): '1055'}),
                      'testdir': ('4cabc8cb-599d-488c-a253-165f71d4e180', {('entry', 'type'): 'collection'}),
                      'testfile': ('cf05727b-73f3-4318-8454-16eaf10f302c', {('entry', 'type'): 'file', ('states', 'size'): '11'})},
                     'found'),
             'kirk': ({}, 'is a file')}
        """
        tree = XMLTree(from_tree =
            ('man:list', [
                ('man:listRequestList', [
                    ('man:listRequestElement', [
                        ('man:requestID', requestID),
                        ('man:LN', LN)
                    ]) for requestID, LN in requests.items()
                ]),
                ('man:neededMetadataList', [
                    ('man:neededMetadataElement', [
                        ('man:section', section),
                        ('man:property', property)
                    ]) for section, property in neededMetadata
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        elements = parse_node(xml.Child().Child().Child(),
            ['requestID', 'entries', 'status'], string = False)
        return dict([
            (   str(requestID), 
                (dict([(str(name), (str(GUID), parse_metadata(metadataList))) for name, (GUID, metadataList) in
                    parse_node(entries, ['name', 'GUID', 'metadataList'], string = False).items()]),
                str(status))
            ) for requestID, (entries, status) in elements.items()
        ])

    def move(self, requests):
        """ Move a file or collection within the global namespace.
        
        move(requests)
        
        requests is a dictionary with requestID as key, and (sourceLN, targetLN, preserveOriginal) as value, where
            sourceLN is the source Logical Name
            targetLN is the target Logical Name
            preserverOriginal is True if we want to create a hardlink
        returns a dictionary with requestID as key and status as value
        
        Example:
        In: {'shoo':['/testfile','/newname',False]}
        Out: {'shoo': ['moved']}
        """
        tree = XMLTree(from_tree =
            ('man:move', [
                ('man:moveRequestList', [
                    ('man:moveRequestElement', [
                        ('man:requestID', requestID),
                        ('man:sourceLN', sourceLN),
                        ('man:targetLN', targetLN),
                        ('man:preserveOriginal', preserveOriginal and true or false)
                    ]) for requestID, (sourceLN, targetLN, preserveOriginal) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(xml.Child().Child().Child(), ['requestID', 'status'], single = False)

    def modify(self, requests):
        tree = XMLTree(from_tree =
            ('man:modify', [
                ('man:modifyRequestList', [
                    ('man:modifyRequestElement', [
                        ('man:changeID', changeID),
                        ('man:LN', LN),
                        ('man:changeType', changeType),
                        ('man:section', section),
                        ('man:property', property),
                        ('man:value', value)
                    ]) for changeID, (LN, changeType, section, property, value) in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = self.xmlnode_class(response)
        return parse_node(node.Child().Child().Child(), ['changeID', 'success'], True)

# This part is copied from client.py and the write() method was changed

class ByteIOClient(Client):

    def __init__(self, url):
        ns = self.NS_class({'rb':rbyteio_uri})
        Client.__init__(self, url, ns)

    def read(self, start_offset = None, bytes_per_block = None, num_blocks = None, stride = None, file = None):
        request = []
        if start_offset is not None:
            request.append(('rb:start-offset', start_offset))
        if bytes_per_block is not None:
            request.append(('rb:bytes-per-block', bytes_per_block))
        if num_blocks is not None:
            request.append(('rb:num-blocks', num_blocks))
        if stride is not None:
            request.append(('rb:stride', stride))
        tree = XMLTree(from_tree = ('rb:read', request))
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        data_encoded = str(xml.Child().Child().Get('transfer-information'))
        if file:
            file.write(base64.b64decode(data_encoded))
            file.close()
        else:
            return base64.b64decode(data_encoded)

    def write(self, data, start_offset = None, bytes_per_block = None, stride = None):
        if isinstance(data,file):
            data = data.read()
        out = self.create_soap_envelope(self.ns)
        request_node = out.Child().NewChild('rb:write')
        if start_offset is not None:
            request_node.NewChild('rb:start-offset').Set(str(start_offset))
        if bytes_per_block is not None:
            request_node.NewChild('rb:bytes-per-block').Set(str(bytes_per_block))
        if stride is not None:
            request_node.NewChild('rb:stride').Set(str(stride))
        transfer_node = request_node.NewChild('rb:transfer-information')
        transfer_node.node.attributes['transfer-mechanism'] = byteio_simple_uri
        encoded_data = base64.b64encode(data)
        transfer_node.Set(encoded_data)
        resp, _, _ = self.call_raw(out)
        return resp

# This part is copied from manager_client.py but then changed

args = sys.argv[1:]
if len(args) > 0 and args[0] == '-x':
    args.pop(0)
    print_xml = True
else:
    print_xml = False

try:
    manager_url = os.environ['ARC_MANAGER_URL']
    #print 'ARC_MANAGER_URL =', manager_url
except:
    manager_url = 'http://localhost:60000/Manager'
    print 'ARC_MANAGER_URL environment variable not found, using', manager_url
    
manager = ManagerClient(manager_url, print_xml)

if len(args) == 0 or args[0][0:3] not in ['sta', 'mak', 'lis', 'mov', 'put', 'get', 'del', 'add', 'mod']:
    print 'Supported methods: stat, make[Collection], list, move, put[File], get[File], del[File]' 
else:
    command = args.pop(0)[0:3]
    if command == 'sta':
        if len(args) < 1:
            print 'Usage: stat <LN> [<LN> ...]'
        else:
            request = dict([(i, args[i]) for i in range(len(args))])
            #print 'stat', request
            stat = manager.stat(request)
            #print stat
            for i,s in stat.items():
                print '%s:' % args[int(i)]
                c = {}
                for k,v in s.items():
                    sect, prop = k
                    c[sect] = c.get(sect,[])
                    c[sect].append((prop, v))
                for k, vs in c.items():
                    print k
                    for p, v in vs:
                        print '  %s: %s' % (p, v)
    elif command == 'del':
        if len(args) < 1:
            print 'Usage: delFile <LN> [<LN> ...]'
        else:
            request = dict([(str(i), args[i]) for i in range(len(args))])
            #print 'delFile', request
            response = manager.delFile(request)
            #print response
            for i in request.keys():
                print '%s: %s' % (request[i], response[i])
    elif command == 'get':
        if len(args) < 1:
            print 'Usage: getFile <source LN> [<target filename>]'
        else:
            LN = args[0]
            try:
                filename = args[1]
            except:
                _, _, filename = splitLN(LN)
            if os.path.exists(filename):
                print '"%s"' % filename, 'already exists'
            else:
                f = file(filename, 'wb')
                request = {'0' : (LN, ['byteio'])}
                #print 'getFile', request
                response = manager.getFile(request)
                #print response
                success, turl, protocol = response['0']
                if success == 'done':
                    print 'Downloading from', turl, 'to', filename
                    ByteIOClient(turl).read(file = f)
                    print LN, 'downloaded to', filename
                else:
                    f.close()
                    os.unlink(filename)
                    print '%s: %s' % (LN, success)
    elif command == 'add':
        if len(args) < 2:
            print 'Usage: addReplica <source filename> <GUID>'
        else:
            filename = args[0]
            requests = {'0' : args[1]}
            protocols = ['byteio']
            #print 'addReplica', requests, protocols
            response = manager.addReplica(requests, protocols)
            #print response
            success, turl, protocol = response['0']
            if success == 'done':
                f = file(filename,'rb')
                print 'Uploading from', filename, 'to', turl
                ByteIOClient(turl).write(f)
                print filename, 'added to', args[1]
            else:
                print '%s: %s' % (filename, success)
    elif command == 'put':
        if len(args) < 2:
            print 'Usage: putFile <source filename> <target LN>'
        else:
            filename = args[0]
            size = os.path.getsize(filename)
            f = file(filename,'rb')
            checksum = create_checksum(f, 'md5')
            LN = args[1]
            if LN.endswith('/'):
                LN = LN + filename.split('/')[-1]
            metadata = {('states', 'size') : size, ('states', 'checksum') : checksum,
                    ('states', 'checksumType') : 'md5', ('states', 'neededReplicas') : 2}
            request = {'0': (LN, metadata, ['byteio'])}
            #print 'putFile', request
            response = manager.putFile(request)
            #print response
            success, turl, protocol = response['0']
            if success == 'done':
                f = file(filename,'rb')
                print 'Uploading from', filename, 'to', turl
                ByteIOClient(turl).write(f)
                print filename, 'uploaded to', LN
            else:
                print '%s: %s' % (filename, success)
    elif command == 'mak':
        if len(args) < 1:
            print 'Usage: makeCollection <LN>'
        else:
            request = {'0': (args[0], {('states', 'closed') : false})}
            #print 'makeCollection', request
            response = manager.makeCollection(request)
            #print response
            print response['0']
    elif command == 'lis':
        if len(args) < 1:
            print 'Usage: list <LN> [<LN> ...]'
        else:
            request = dict([(str(i), args[i]) for i in range(len(args))])
            more_than_one = len(args) > 1
            #print 'list', request
            response = manager.list(request,[('entry','')])
            #print response
            for rID, (entries, status) in response.items():
                if status == 'found':
                    if more_than_one:
                        print '%s:' % request[rID]
                    for name, (GUID, metadata) in entries.items():
                        if more_than_one:
                            print '\t',
                        print '%s%s<%s>' % (name, ' '*(20-len(name)), metadata.get(('entry', 'type'),'unknown'))
                elif status == 'is a file':
                    _, _, name = splitLN(request[rID])
                    print '%s%s<%s>' % (name, ' '*(20-len(name)), 'file')
                else:
                    print '%s: %s' % (request[rID], status)
    elif command == 'mov':
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
            #print 'move', request
            response = manager.move(request)
            #print response
            print response['0'][0]
    elif command == 'mod':
        if len(args) < 5:
            print 'Usage: modify <LN> <changeType> <section> <property> <value>'
        else:
            request = {'0' : args}
            #print 'modify', request
            print manager.modify(request)['0']