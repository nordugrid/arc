from storage.common import ahash_uri, librarian_uri, bartender_uri, rbyteio_uri, byteio_simple_uri, shepherd_uri, parse_url
from storage.common import parse_metadata, create_metadata, true, false, get_data_node, get_child_nodes, node_to_data, parse_node, parse_to_dict

#Gateway Services
from storage.common import gateway_uri, transfer_uri, externalInfo_uri

from storage.xmltree import XMLTree
from xml.dom.minidom import parseString
import arc
import base64
import time
import sys
import socket

class Client:
    """ Base Client class for sending SOAP messages to services """

    NS_class = arc.NS

    def __init__(self, url, ns, print_xml = False, xmlnode_class = arc.XMLNode, ssl_config = {}):
        """ The constructor of the Client class.
        
        Client(url, ns, print_xml = false)
        
        url is the URL of the service
        ns contains the namespaces we want to use with each message
        print_xml is for debugging, prints all the SOAP messages to the screen
        """
        proto, host, port, path = parse_url(url)
        self.ssl_config = {}
        if proto == 'https':
            self.ssl_config = ssl_config
        self.host = host
        self.port = port
        self.path = path
        self.ns = ns
        self.url = url
        self.print_xml = print_xml
        self.xmlnode_class = xmlnode_class
        self.connection = None

    def call(self, tree, return_tree_only = False):
        """ Create a SOAP message from an XMLTree and send it to the service.
        
        call(tree, return_tree_only = False)
        
        tree is an XMLTree object containing the content of the request
        return_tree_only indicates that we only need to put the response into an XMLTree
        """
        # create a new PayloadSOAP object with the given namespace
        out = arc.PayloadSOAP(self.ns)
        # add the content of the XMLTree to the XMLNode of the SOAP object
        tree.add_to_node(out)
        if self.print_xml:
            msg = out.GetXML()
            print 'Request:'
            print XMLTree(out).pretty_xml(indent = '    ', prefix = '        #   ')
            print
        # call the service and get back the response, and the HTTP status
        resp, status, reason = self.call_raw(out)
        if self.print_xml:
            print 'Response:'
            try:
                print XMLTree(from_string = resp).pretty_xml(indent = '    ', prefix = '        #   ')
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
        timeout = 60
        start = time.time()
        while True:
            try:
                if self.connection:
                    h = self.connection
                    self.connection = None
                else:
                    if self.ssl_config:
                        h = httplib.HTTPSConnection(self.host, self.port, key_file = self.ssl_config.get('key_file', None), cert_file = self.ssl_config.get('cert_file', None))
                    else:
                        # create an HTTP connection
                        h = httplib.HTTPConnection(self.host, self.port)
                    # print 'new', h, self.host, self.port, self.path, self.ssl_config
                # get the XML from outpayload, and send it as POST
                h.request('POST', self.path, outpayload.GetXML())
                # get the response object
                r = h.getresponse()
                # read the response data
                resp = r.read()
                self.connection = h
                # return the data and the status
                return resp, r.status, r.reason
            except socket.error, e:
                if e[0] != 99:
                    raise
                #print "error connecting to %s:%s/%s" % (self.host, self.port, self.path), e,
                #sys.stdout.flush()
                if time.time() < start + timeout:
                    time.sleep(5)
                    #print "Retrying."
                    #sys.stdout.flush()
                else:
                    #print "Giving up."
                    #sys.stdout.flush()
                    return '<Fault>Cannot connect.</Fault>', None, None


class AHashClient(Client):

    def __init__(self, url, print_xml = False, ssl_config = {}):
        ns = self.NS_class({'ahash': ahash_uri})
        Client.__init__(self, url, ns, print_xml, ssl_config = ssl_config)

    def get_tree(self, IDs, neededMetadata = []):
        tree = XMLTree(from_tree =
            ('ahash:get', [
                ('ahash:neededMetadataList', [
                    ('ahash:neededMetadataElement', [
                        ('ahash:section', section),
                        ('ahash:property', property)
                    ]) for section, property in neededMetadata
                ]),
                ('ahash:IDs', [
                    ('ahash:ID', i) for i in IDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = self.xmlnode_class(msg)
        ahash_prefix = xml.NamespacePrefix(ahash_uri)
        rewrite = {
            ahash_prefix + ':objects' : 'lbr:getResponseList',
            ahash_prefix + ':object' : 'lbr:getResponseElement',
            ahash_prefix + ':ID' : 'lbr:GUID',
            ahash_prefix + ':metadataList' : 'lbr:metadataList',
            ahash_prefix + ':metadata' : 'lbr:metadata',
            ahash_prefix + ':section' : 'lbr:section',
            ahash_prefix + ':property' : 'lbr:property',
            ahash_prefix + ':value' : 'lbr:value'
        }
        return XMLTree(get_data_node(xml), rewrite = rewrite)

    def get(self, IDs, neededMetadata = []):
        tree = XMLTree(from_tree =
            ('ahash:get', [
                ('ahash:neededMetadataList', [
                    ('ahash:neededMetadataElement', [
                        ('ahash:section', section),
                        ('ahash:property', property)
                    ]) for section, property in neededMetadata
                ]),
                ('ahash:IDs', [
                    ('ahash:ID', i) for i in IDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = self.xmlnode_class(msg)
        objects = parse_node(get_data_node(xml), ['ID', 'metadataList'], single = True, string = False)
        return dict([(str(ID), parse_metadata(metadataList)) for ID, metadataList in objects.items()])

    def change(self, changes):
        """ Call the change method of the A-Hash service.
        
        change(changes)

        'changes' is a dictionary of {changeID : (ID, changeType, section, property, value, conditions)}
            where 'conditions' is a dictionary of {conditionID : (conditionType, section, property, value)}
        """
        tree = XMLTree(from_tree =
            ('ahash:change', [
                ('ahash:changeRequestList', [
                    ('ahash:changeRequestElement', [
                        ('ahash:changeID', changeID),
                        ('ahash:ID', ID),
                        ('ahash:changeType', changeType),
                        ('ahash:section', section),
                        ('ahash:property', property),
                        ('ahash:value', value),
                        ('ahash:conditionList', [
                            ('ahash:condition', [
                                ('ahash:conditionID', conditionID),
                                ('ahash:conditionType',conditionType),
                                ('ahash:section',conditionSection),
                                ('ahash:property',conditionProperty),
                                ('ahash:value',conditionValue)
                            ]) for conditionID, (conditionType, conditionSection,
                                        conditionProperty, conditionValue) in conditions.items()
                        ])
                    ]) for changeID, (ID, changeType, section, property, value, conditions) in changes.items()
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['changeID', 'success', 'conditionID'])

class LibrarianClient(Client):
    
    def __init__(self, url, print_xml = False, ssl_config = {}):
        ns = self.NS_class({'lbr':librarian_uri})
        Client.__init__(self, url, ns, print_xml, ssl_config = ssl_config)

    def get(self, GUIDs, neededMetadata = []):
        tree = XMLTree(from_tree =
            ('lbr:get', [
                ('lbr:neededMetadataList', [
                    ('lbr:neededMetadataElement', [
                        ('lbr:section', section),
                        ('lbr:property', property)
                    ]) for section, property in neededMetadata
                ]),
                ('lbr:getRequestList', [
                    ('lbr:getRequestElement', [
                        ('lbr:GUID', i)
                    ]) for i in GUIDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = self.xmlnode_class(msg)
        elements = parse_node(get_data_node(xml),
            ['GUID', 'metadataList'], single = True, string = False)
        return dict([(str(GUID), parse_metadata(metadataList))
            for GUID, metadataList in elements.items()])

    def traverseLN(self, requests):
        tree = XMLTree(from_tree =
            ('lbr:traverseLN', [
                ('lbr:traverseLNRequestList', [
                    ('lbr:traverseLNRequestElement', [
                        ('lbr:requestID', rID),
                        ('lbr:LN', LN)
                    ]) for rID, LN in requests.items()
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = self.xmlnode_class(msg)
        list_node = get_data_node(xml)
        list_number = list_node.Size()
        elements = {}
        for i in range(list_number):
            element_node = list_node.Child(i)
            requestID = str(element_node.Get('requestID'))
            traversedlist_node = element_node.Get('traversedList')
            traversedlist_number = traversedlist_node.Size()
            traversedlist = []
            for j in range(traversedlist_number):
                tle_node = traversedlist_node.Child(j)
                traversedlist.append((str(tle_node.Get('LNPart')), str(tle_node.Get('GUID'))))
            wasComplete = str(element_node.Get('wasComplete')) == true
            traversedLN = str(element_node.Get('traversedLN'))
            restLN = str(element_node.Get('restLN'))
            GUID = str(element_node.Get('GUID'))
            metadatalist_node = element_node.Get('metadataList')
            metadata = parse_metadata(metadatalist_node)
            elements[requestID] = (metadata, GUID, traversedLN, restLN, wasComplete, traversedlist)
        return elements

    def new(self, requests):
        tree = XMLTree(from_tree =
            ('lbr:new', [
                ('lbr:newRequestList', [
                    ('lbr:newRequestElement', [
                        ('lbr:requestID', requestID),
                        ('lbr:metadataList', create_metadata(metadata, 'lbr'))
                    ]) for requestID, metadata in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = self.xmlnode_class(response)
        return parse_node(get_data_node(node), ['requestID', 'GUID', 'success'])

    def modifyMetadata(self, requests):
        tree = XMLTree(from_tree =
            ('lbr:modifyMetadata', [
                ('lbr:modifyMetadataRequestList', [
                    ('lbr:modifyMetadataRequestElement', [
                        ('lbr:changeID', changeID),
                        ('lbr:GUID', GUID),
                        ('lbr:changeType', changeType),
                        ('lbr:section', section),
                        ('lbr:property', property),
                        ('lbr:value', value)
                    ]) for changeID, (GUID, changeType, section, property, value) in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = self.xmlnode_class(response)
        return parse_node(get_data_node(node), ['changeID', 'success'], True)

    def remove(self, requests):
        tree = XMLTree(from_tree =
            ('lbr:remove', [
                ('lbr:removeRequestList', [
                    ('lbr:removeRequestElement', [
                        ('lbr:requestID', requestID),
                        ('lbr:GUID', GUID)
                    ]) for requestID, GUID in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = self.xmlnode_class(response)
        return parse_node(get_data_node(node), ['requestID', 'success'], True)

    def report(self, serviceID, filelist):
        tree = XMLTree(from_tree =
            ('lbr:report', [
                ('lbr:serviceID', serviceID),
                ('lbr:filelist', [
                    ('lbr:file', [
                        ('lbr:GUID', GUID),
                        ('lbr:referenceID', referenceID),
                        ('lbr:state', state)
                    ]) for GUID, referenceID, state in filelist
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = self.xmlnode_class(response)
        try:
            return int(str(node.Child().Child().Get('nextReportTime')))
        except:
            return None

class BartenderClient(Client):
    """ Client for the Bartender service. """
    
    def __init__(self, url, print_xml = False, ssl_config = {}):
        """ Constructior of the client.
        
        BartenderClient(url, print_xml = False)
        
        url is the URL of the Bartender service
        if print_xml is true this will print the SOAP messages
        """
        # sets the namespace
        ns = self.NS_class({'bar':bartender_uri})
        # calls the superclass' constructor
        Client.__init__(self, url, ns, print_xml, ssl_config = ssl_config)

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
                     ('locations', 'http://localhost:60000/Shepherd 00d6388c-42df-441c-8d9f-bd78c2c51667'): 'alive',
                     ('locations', 'http://localhost:60000/Shepherd 51c9b49a-1472-4389-90d2-0f18b960fe29'): 'alive',
                     ('states', 'checksum'): '0927c28a393e8834aa7b838ad8a69400',
                     ('states', 'checksumType'): 'md5',
                     ('states', 'neededReplicas'): '5',
                     ('states', 'size'): '11'}}
        """
        tree = XMLTree(from_tree =
            ('bar:stat', [
                ('bar:statRequestList', [
                    ('bar:statRequestElement', [
                        ('bar:requestID', requestID),
                        ('bar:LN', LN) 
                    ]) for requestID, LN in requests.items()
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = self.xmlnode_class(msg)
        elements = parse_node(get_data_node(xml),
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
            ('bar:getFile', [
                ('bar:getFileRequestList', [
                    ('bar:getFileRequestElement', [
                        ('bar:requestID', rID),
                        ('bar:LN', LN)
                    ] + [
                        ('bar:protocol', protocol) for protocol in protocols
                    ]) for rID, (LN, protocols) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'success', 'TURL', 'protocol'])
    
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
            ('bar:putFile', [
                ('bar:putFileRequestList', [
                    ('bar:putFileRequestElement', [
                        ('bar:requestID', rID),
                        ('bar:LN', LN),
                        ('bar:metadataList', create_metadata(metadata, 'bar')),
                    ] + [
                        ('bar:protocol', protocol) for protocol in protocols
                    ]) for rID, (LN, metadata, protocols) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'success', 'TURL', 'protocol'])

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
            ('bar:delFile', [
                ('bar:delFileRequestList', [
                    ('bar:delFileRequestElement', [
                        ('bar:requestID', requestID),
                        ('bar:LN', LN) 
                    ]) for requestID, LN in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'success'], single = True)


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
            ('bar:addReplica', [
                ('bar:addReplicaRequestList', [
                    ('bar:putReplicaRequestElement', [
                        ('bar:requestID', rID),
                        ('bar:GUID', GUID),
                    ]) for rID, GUID in requests.items()
                ])
            ] + [
                ('bar:protocol', protocol) for protocol in protocols
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'success', 'TURL', 'protocol'])

    def unmakeCollection(self, requests):
        """docstring for unmakeCollection"""
        tree = XMLTree(from_tree =
            ('bar:unmakeCollection', [
                ('bar:unmakeCollectionRequestList', [
                    ('bar:unmakeCollectionRequestElement', [
                        ('bar:requestID', rID),
                        ('bar:LN', LN),
                    ]) for rID, LN in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'success'], single = True)

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
            ('bar:makeCollection', [
                ('bar:makeCollectionRequestList', [
                    ('bar:makeCollectionRequestElement', [
                        ('bar:requestID', rID),
                        ('bar:LN', LN),
                        ('bar:metadataList', create_metadata(metadata, 'bar'))
                    ]) for rID, (LN, metadata) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'success'], single = True)

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
            ('bar:list', [
                ('bar:listRequestList', [
                    ('bar:listRequestElement', [
                        ('bar:requestID', requestID),
                        ('bar:LN', LN)
                    ]) for requestID, LN in requests.items()
                ]),
                ('bar:neededMetadataList', [
                    ('bar:neededMetadataElement', [
                        ('bar:section', section),
                        ('bar:property', property)
                    ]) for section, property in neededMetadata
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        elements = parse_node(get_data_node(xml),
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
            ('bar:move', [
                ('bar:moveRequestList', [
                    ('bar:moveRequestElement', [
                        ('bar:requestID', requestID),
                        ('bar:sourceLN', sourceLN),
                        ('bar:targetLN', targetLN),
                        ('bar:preserveOriginal', preserveOriginal and true or false)
                    ]) for requestID, (sourceLN, targetLN, preserveOriginal) in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'status'], single = False)

    def modify(self, requests):
        tree = XMLTree(from_tree =
            ('bar:modify', [
                ('bar:modifyRequestList', [
                    ('bar:modifyRequestElement', [
                        ('bar:changeID', changeID),
                        ('bar:LN', LN),
                        ('bar:changeType', changeType),
                        ('bar:section', section),
                        ('bar:property', property),
                        ('bar:value', value)
                    ]) for changeID, (LN, changeType, section, property, value) in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = self.xmlnode_class(response)
        return parse_node(get_data_node(node), ['changeID', 'success'], True)

class ShepherdClient(Client):

    def __init__(self, url, print_xml = False, ssl_config = {}):
        ns = self.NS_class({'she':shepherd_uri})
        Client.__init__(self, url, ns, print_xml, ssl_config = ssl_config)

    def _putget(self, putget, requests):
        tree = XMLTree(from_tree =
            ('she:' + putget, [
                ('she:' + putget + 'RequestList', [
                    ('she:' + putget + 'RequestElement', [
                        ('she:requestID', requestID),
                        ('she:' + putget + 'RequestDataList', [
                            ('she:' + putget + 'RequestDataElement', [
                                ('she:property', property),
                                ('she:value', value)
                            ]) for property, value in requestData
                        ])
                    ]) for requestID, requestData in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        try:
            response = dict([
                (str(node.Get('requestID')), [
                    (str(n.Get('property')), str(n.Get('value')))
                        for n in get_child_nodes(node.Get(putget + 'ResponseDataList'))
                ]) for node in get_child_nodes(get_data_node(xml))])
            return response
        except:
            raise Exception, msg

    def put(self, requests):
        return self._putget('put', requests)

    def get(self, requests):
        return self._putget('get', requests)

    def delete(self, requests):
        tree = XMLTree(from_tree =
            ('she:delete', [
                ('she:deleteRequestList', [
                    ('she:deleteRequestElement', [
                        ('she:requestID', requestID),
                        ('she:referenceID', referenceID)
                    ]) for requestID, referenceID in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_node(get_data_node(xml), ['requestID', 'status'])
        

    def stat(self, requests):
        tree = XMLTree(from_tree =
            ('she:stat', [
                ('she:statRequestList', [
                    ('she:statRequestElement', [
                        ('she:requestID', requestID),
                        ('she:referenceID', referenceID)
                    ]) for requestID, referenceID in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = self.xmlnode_class(msg)
        return parse_to_dict(get_data_node(xml),
            ['requestID', 'referenceID', 'state', 'checksumType', 'checksum', 'acl', 'size', 'GUID', 'localID'])

    def toggleReport(self, doReporting):
        tree = XMLTree(from_tree =
            ('she:toggleReport', [
                ('she:doReporting', doReporting and true or false)
            ])
        )
        return self.call(tree, True)

class NotifyClient(Client):

    def __init__(self, url, print_xml = False, ssl_config = {}):
        ns = self.NS_class({'she':shepherd_uri})
        Client.__init__(self, url, ns, print_xml, ssl_config = ssl_config)

    def notify(self, subject, state):
        tree = XMLTree(from_tree =
            ('she:notify', [
                ('she:subject', subject),
                ('she:state', state)
            ])
        )
        msg, _, _ = self.call(tree)
        return msg

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
        out = arc.PayloadSOAP(self.ns)
        request_node = out.NewChild('rb:write')
        if start_offset is not None:
            request_node.NewChild('rb:start-offset').Set(str(start_offset))
        if bytes_per_block is not None:
            request_node.NewChild('rb:bytes-per-block').Set(str(bytes_per_block))
        if stride is not None:
            request_node.NewChild('rb:stride').Set(str(stride))
        transfer_node = request_node.NewChild('rb:transfer-information')
        transfer_node.NewAttribute('transfer-mechanism').Set(byteio_simple_uri)
        encoded_data = base64.b64encode(data)
        transfer_node.Set(encoded_data)
        resp, _, _ = self.call_raw(out)
        return resp

class GatewayClient(Client):

        def __init__(self, url):

                ns = arc.NS({'gateway':gateway_uri})
                # calls the superclass' constructor
                Client.__init__(self, url, ns)

        def getFile(self, requests):


                """requests will come with the source and the destination URLS of the file. """

                sourceURL = requests[0]
                destinationURL = requests[1]
                tree = XMLTree(from_tree = ('gateway:getFile', [
                                                ('gateway:URLs',[
                                                        ('gateway:sourceURL',sourceURL),
                                                        ('gateway:destinationURL',destinationURL)
                                                                ])
                                                        ]))
                print tree
                msg, _, _ = self.call(tree)
                xml = arc.XMLNode(msg)
                print xml
                elements = parse_node(xml.Child().Child().Child(),
                ['requestID', 'success','TURL'], string = True)
                return elements

        def putFile(self, requests):


                """requests will come with the source and the destination URLS of the file. """

                sourceURL = requests[0]
                destinationURL = requests[1]
                tree = XMLTree(from_tree = ('gateway:putFile', [
                                                ('gateway:URLs',[
                                                        ('gateway:sourceURL',sourceURL),
                                                        ('gateway:destinationURL',destinationURL)
                                                                ])
                                                        ]))
                print tree
                msg, _, _ = self.call(tree)
                xml = arc.XMLNode(msg)
                elements = parse_node(xml.Child().Child().Child(),
                ['requestID', 'success','status'], string = True)
                return elements

        def list(self, requests, flags = '' ):

                """requests contain the path of the file or directory.
                options contain whether user needs long listing or not."""

                tree = XMLTree(from_tree = ('gateway:list', [
                                                ('gateway:URLs',[
                                                        ('gateway:externalURL', requests),
                                                        ('gateway:flags', flags)
                                                                ])
                                                         ]))
                msg, _, _ = self.call(tree)
                xml = arc.XMLNode(msg)
                elements = parse_to_dict(get_data_node(xml),
                        ['requestID', 'status','output'])

                #elements = parse_node(xml.Child().Child().Child(),
                #['ID', 'status','output'], string = True)
                #return elements
		for ele in elements:
			print "\n "
			print "External URL: "+ele
			print "Output >>>>>>>>>"
			print "---------------"
			print elements[ele]['output']
			print "---------------"
			print "Status: "+str(elements[ele]['status'])
			print "---------------"

class ExternalStorageInformationClient(Client):

        def __init__(self, url):

                ns = arc.NS({'externalStorageInformation':externalInfo_uri})
                # calls the superclass' constructor
                Client.__init__(self, url, ns)

        def getInfo(self, request):

                print "External Storage getInfo function"
                tree = XMLTree(from_tree = ('externalStorageInformation:getInfo', [
                                                        ('externalStorageInformation:store',[
                                                                ('externalStorageInformation:name', request)
                                                                        ])
                                                                ]))
                msg, _, _  = self.call(tree)
                xml = arc.XMLNode(msg)
                elements = parse_node(xml.Child().Child().Child(),
                ['hostname', 'protocol', 'port'], string = True)
                return elements

class TransferClient(Client):

        def __init__(self, url):

                ns = arc.NS({'transfer':transfer_uri})
                # calls the superclass' constructor
                Client.__init__(self, url, ns)

        def transferData(self,request):

                print "Transfer test function"
                print request
                tree = XMLTree(from_tree = ('transfer:transferData', [
                                                        ('transfer:URLs',[
                                                                ('transfer:hostname', res),
                                                                ('transfer:flags', request[res][0]),
								('transfer:protocol',request[res][1]),
								('transfer:port',request[res][2]),
                                                                ('transfer:path',request[res][3])
								        ])for res in request.keys()
                                                                ]))

                msg, _, _ = self.call(tree)
                xml = arc.XMLNode(msg)
                elements = parse_node(xml.Child().Child().Child(),
                ['ID', 'status', 'output'], string = True)
                return elements

####################################################

