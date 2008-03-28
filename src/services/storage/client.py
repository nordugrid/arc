from storage.common import hash_uri, catalog_uri, manager_uri, rbyteio_uri, byteio_simple_uri, element_uri
from storage.common import parse_metadata, create_metadata, true, false, get_child_nodes, node_to_data, parse_node, parse_to_dict
from storage.xmltree import XMLTree
from xml.dom.minidom import parseString
import arc
import base64

class Client:

    def __init__(self, url, ns, print_xml = False):
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
        self.print_xml = print_xml

    def call(self, tree, return_tree_only = False):
        out = arc.PayloadSOAP(self.ns)
        tree.add_to_node(out)
        if self.print_xml:
            msg = out.GetXML()
            print 'Request:'
            print parseString(msg).toprettyxml()
            print
        resp, status, reason = self.call_raw(out)
        if self.print_xml:
            print 'Response:'
            try:
                print parseString(resp).toprettyxml()
            except:
                print resp
            print
        if return_tree_only:
            return XMLTree(from_string = resp, forget_namespace = True).get_trees('///')[0]
        else:
            return resp, status, reason

    def call_raw(self, outpayload):
        import httplib
        h = httplib.HTTPConnection(self.host, self.port)
        h.request('POST', self.path, outpayload.GetXML())
        r = h.getresponse()
        resp = r.read()
        return resp, r.status, r.reason


class HashClient(Client):

    def __init__(self, url, print_xml = False):
        ns = arc.NS({'hash': hash_uri})
        Client.__init__(self, url, ns, print_xml)

    def get_tree(self, IDs, neededMetadata = []):
        tree = XMLTree(from_tree =
            ('hash:get', [
                ('hash:neededMetadataList', [
                    ('hash:neededMetadataElement', [
                        ('hash:section', section),
                        ('hash:property', property)
                    ]) for section, property in neededMetadata
                ]),
                ('hash:IDs', [
                    ('hash:ID', i) for i in IDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        hash_prefix = xml.NamespacePrefix(hash_uri)
        rewrite = {
            hash_prefix + ':objects' : 'cat:getResponseList',
            hash_prefix + ':object' : 'cat:getResponseElement',
            hash_prefix + ':ID' : 'cat:GUID',
            hash_prefix + ':metadataList' : 'cat:metadataList',
            hash_prefix + ':metadata' : 'cat:metadata',
            hash_prefix + ':section' : 'cat:section',
            hash_prefix + ':property' : 'cat:property',
            hash_prefix + ':value' : 'cat:value'
        }
        return XMLTree(xml.Child().Child().Child(), rewrite = rewrite)

    def get(self, IDs, neededMetadata = []):
        tree = XMLTree(from_tree =
            ('hash:get', [
                ('hash:neededMetadataList', [
                    ('hash:neededMetadataElement', [
                        ('hash:section', section),
                        ('hash:property', property)
                    ]) for section, property in neededMetadata
                ]),
                ('hash:IDs', [
                    ('hash:ID', i) for i in IDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        objects = parse_node(xml.Child().Child().Child(), ['ID', 'metadataList'], single = True, string = False)
        return dict([(str(ID), parse_metadata(metadataList)) for ID, metadataList in objects.items()])

    def change(self, changes):
        """ Call the change method of the Hash service.
        
        change(changes)

        'changes' is a dictionary of {changeID : (ID, changeType, section, property, value, conditions)}
            where 'conditions' is a dictionary of {conditionID : (conditionType, section, property, value)}
        """
        tree = XMLTree(from_tree =
            ('hash:change', [
                ('hash:changeRequestList', [
                    ('hash:changeRequestElement', [
                        ('hash:changeID', changeID),
                        ('hash:ID', ID),
                        ('hash:changeType', changeType),
                        ('hash:section', section),
                        ('hash:property', property),
                        ('hash:value', value),
                        ('hash:conditionList', [
                            ('hash:condition', [
                                ('hash:conditionID', conditionID),
                                ('hash:conditionType',conditionType),
                                ('hash:section',conditionSection),
                                ('hash:property',conditionProperty),
                                ('hash:value',conditionValue)
                            ]) for conditionID, (conditionType, conditionSection,
                                        conditionProperty, conditionValue) in conditions.items()
                        ])
                    ]) for changeID, (ID, changeType, section, property, value, conditions) in changes.items()
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        return parse_node(xml.Child().Child().Child(), ['changeID', 'success', 'conditionID'])

class CatalogClient(Client):
    
    def __init__(self, url, print_xml = False):
        ns = arc.NS({'cat':catalog_uri})
        Client.__init__(self, url, ns, print_xml)

    def get(self, GUIDs, neededMetadata = []):
        tree = XMLTree(from_tree =
            ('cat:get', [
                ('cat:neededMetadataList', [
                    ('cat:neededMetadataElement', [
                        ('cat:section', section),
                        ('cat:property', property)
                    ]) for section, property in neededMetadata
                ]),
                ('cat:getRequestList', [
                    ('cat:getRequestElement', [
                        ('cat:GUID', i)
                    ]) for i in GUIDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        elements = parse_node(xml.Child().Child().Child(),
            ['GUID', 'metadataList'], single = True, string = False)
        return dict([(str(GUID), parse_metadata(metadataList))
            for GUID, metadataList in elements.items()])

    def traverseLN(self, requests):
        tree = XMLTree(from_tree =
            ('cat:traverseLN', [
                ('cat:traverseLNRequestList', [
                    ('cat:traverseLNRequestElement', [
                        ('cat:requestID', rID),
                        ('cat:LN', LN)
                    ]) for rID, LN in requests.items()
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        list_node = xml.Child().Child().Child()
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
            ('cat:new', [
                ('cat:newRequestList', [
                    ('cat:newRequestElement', [
                        ('cat:requestID', requestID),
                        ('cat:metadataList', create_metadata(metadata, 'cat'))
                    ]) for requestID, metadata in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = arc.XMLNode(response)
        return parse_node(node.Child().Child().Child(), ['requestID', 'GUID', 'success'])

    def modifyMetadata(self, requests):
        tree = XMLTree(from_tree =
            ('cat:modifyMetadata', [
                ('cat:modifyMetadataRequestList', [
                    ('cat:modifyMetadataRequestElement', [
                        ('cat:changeID', changeID),
                        ('cat:GUID', GUID),
                        ('cat:changeType', changeType),
                        ('cat:section', section),
                        ('cat:property', property),
                        ('cat:value', value)
                    ]) for changeID, (GUID, changeType, section, property, value) in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = arc.XMLNode(response)
        return parse_node(node.Child().Child().Child(), ['changeID', 'success'], True)

    def remove(self, requests):
        tree = XMLTree(from_tree =
            ('cat:remove', [
                ('cat:removeRequestList', [
                    ('cat:removeRequestElement', [
                        ('cat:requestID', requestID),
                        ('cat:GUID', GUID)
                    ]) for requestID, GUID in requests.items()
                ])
            ])
        )
        response, _, _ = self.call(tree)
        node = arc.XMLNode(response)
        return parse_node(node.Child().Child().Child(), ['requestID', 'success'], True)

class ManagerClient(Client):
    
    def __init__(self, url, print_xml = False):
        ns = arc.NS({'man':manager_uri})
        Client.__init__(self, url, ns, print_xml)

    def stat(self, requests):
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
        xml = arc.XMLNode(msg)
        elements = parse_node(xml.Child().Child().Child(),
            ['requestID', 'metadataList'], single = True, string = False)
        return dict([(str(requestID), parse_metadata(metadataList))
            for requestID, metadataList in elements.items()])

    def putFile(self, requests):
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
        return self.call(tree, True)

    def makeCollection(self, requests):
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
        return self.call(tree, True)

    def list(self, requests, neededMetadata = []):
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
        xml = arc.XMLNode(msg)
        elements = parse_node(xml.Child().Child().Child(),
            ['requestID', 'entries'], single = True, string = False)
        return dict([
            (   str(requestID),
                dict([(str(name), (str(GUID), parse_metadata(metadataList))) for name, (GUID, metadataList) in
                    parse_node(entries, ['name', 'GUID', 'metadataList'], string = False).items()])
            ) for requestID, entries in elements.items()
        ])

    def move(self, requests):
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
        xml = arc.XMLNode(msg)
        return msg

class ElementClient(Client):

    def __init__(self, url, print_xml = False):
        ns = arc.NS({'se':element_uri})
        Client.__init__(self, url, ns, print_xml)

    def _putget(self, putget, requests):
        tree = XMLTree(from_tree =
            ('se:' + putget, [
                ('se:' + putget + 'RequestList', [
                    ('se:' + putget + 'RequestElement', [
                        ('se:requestID', requestID),
                        ('se:' + putget + 'RequestDataList', [
                            ('se:' + putget + 'RequestDataElement', [
                                ('se:property', property),
                                ('se:value', value)
                            ]) for property, value in requestData
                        ])
                    ]) for requestID, requestData in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = arc.XMLNode(msg)
        try:
            response = dict([
                (str(node.Get('requestID')), [
                    (str(n.Get('property')), str(n.Get('value')))
                        for n in get_child_nodes(node.Get(putget + 'ResponseDataList'))
                ]) for node in get_child_nodes(xml.Child().Child().Child())])
            return response
        except:
            raise Exception, msg

    def put(self, requests):
        return self._putget('put', requests)

    def get(self, requests):
        return self._putget('get', requests)

    def stat(self, requests):
        tree = XMLTree(from_tree =
            ('se:stat', [
                ('se:statRequestList', [
                    ('se:statRequestElement', [
                        ('se:requestID', requestID),
                        ('se:referenceID', referenceID)
                    ]) for requestID, referenceID in requests.items()
                ])
            ])
        )
        msg, _, _ = self.call(tree)
        xml = arc.XMLNode(msg)
        return parse_to_dict(xml.Child().Child().Child(),
            ['requestID', 'referenceID', 'state', 'checksumType', 'checksum', 'acl', 'size', 'GUID', 'localID'])


class NotifyClient(Client):

    def __init__(self, url, print_xml = False):
        ns = arc.NS({'se':element_uri})
        Client.__init__(self, url, ns, print_xml)

    def notify(self, subject, state):
        tree = XMLTree(from_tree =
            ('se:notify', [
                ('se:subject', subject),
                ('se:state', state)
            ])
        )
        msg, _, _ = self.call(tree)
        return msg

class ByteIOClient(Client):

    def __init__(self, url):
        ns = arc.NS({'rb':rbyteio_uri})
        Client.__init__(self, url, ns)

    def read(self, start_offset = None, bytes_per_block = None, num_blocks = None, stride = None):
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
        xml = arc.XMLNode(msg)
        data_encoded = str(xml.Child().Child().Get('transfer-information'))
        return base64.b64decode(data_encoded)

    def write(self, data, start_offset = None, bytes_per_block = None, stride = None):
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


