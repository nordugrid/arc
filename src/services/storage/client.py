from storage.common import hash_uri, catalog_uri, manager_uri, parse_metadata, true
from storage.xmltree import XMLTree
from xml.dom.minidom import parseString
import arc

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
        msg = out.GetXML()
        if self.print_xml:
            print 'Request:'
            print parseString(msg).toprettyxml()
            print
        import httplib
        h = httplib.HTTPConnection(self.host, self.port)
        h.request('POST', self.path, msg)
        r = h.getresponse()
        resp = r.read()
        if self.print_xml:
            print 'Response:'
            print parseString(resp).toprettyxml()
            print
        if return_tree_only:
            return XMLTree(from_string = resp, forget_namespace = True).get_trees('///')[0]
        else:
            return resp, r.status, r.reason

class HashClient(Client):

    def __init__(self, url, print_xml = False):
        ns = arc.NS({'hash': hash_uri})
        Client.__init__(self, url, ns, print_xml)

    def get_tree(self, IDs):
        tree = XMLTree(from_tree =
            ('hash:get',
                [('hash:IDs',
                    [('hash:ID', i) for i in IDs]
                )]
            ))
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

    def get(self, IDs):
        tree = XMLTree(from_tree =
            ('hash:get',
                [('hash:IDs',
                    [('hash:ID', i) for i in IDs]
                )]
            ))
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        objects_node = xml.Child().Child().Child()
        objects_number = objects_node.Size()
        objects = []
        for i in range(objects_number):
            obj_node = objects_node.Child(i)
            ID = str(obj_node.Get('ID'))
            metadatalist_node = obj_node.Get('metadataList')
            metadata = parse_metadata(metadatalist_node)
            objects.append((ID,metadata))
        return dict(objects)

    def change(self, changes):
        """ Call the change method of the Hash service.
        
        change(changes)

        'changes' is a dictionary of {changeID : (ID, changeType, section, property, value, conditions)}
            where 'conditions' is a list of (conditionID, conditionType, section, property, value)
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
        responses_node = xml.Child().Child().Child()
        responses_number = responses_node.Size()
        responses = []
        for i in range(responses_number):
            response_node = responses_node.Child(i)
            responses.append((str(response_node.Get('changeID')),
                (str(response_node.Get('success')), str(response_node.Get('conditionID')))))
        return dict(responses)

class CatalogClient(Client):
    
    def __init__(self, url, print_xml = False):
        ns = arc.NS({'cat':catalog_uri})
        Client.__init__(self, url, ns, print_xml)

    def get(self, GUIDs):
        tree = XMLTree(from_tree =
            ('cat:get', [
                ('cat:getRequestList', [
                    ('cat:getRequestElement', [
                        ('cat:GUID', i)
                    ]) for i in GUIDs
                ])
            ])
        )
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        list_node = xml.Child().Child().Child()
        list_number = list_node.Size()
        elements = []
        for i in range(list_number):
            element_node = list_node.Child(i)
            GUID = str(element_node.Get('GUID'))
            metadatalist_node = element_node.Get('metadataList')
            metadata = parse_metadata(metadatalist_node)
            elements.append((GUID,metadata))
        return dict(elements)

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
        print list_node.GetXML()
        elements = []
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
            elements.append((requestID, (metadata, GUID, traversedLN, restLN, wasComplete, traversedlist)))
        return dict(elements)

    def newCollection(self, requests):
        tree = XMLTree(from_tree =
            ('cat:newCollection', [
                ('cat:newCollectionRequestList', [
                    ('cat:newCollectionRequestElement', [
                        ('cat:requestID', requestID),
                        ('cat:metadataList', [
                            ('cat:metadataList', [
                                ('cat:metadata', [
                                    ('cat:section', section),
                                    ('cat:property', property),
                                    ('cat:value', value)
                                ]) for ((section, property), value) in metadata.items()
                            ])
                        ])
                    ]) for requestID, metadata in requests.items()
                ])
            ])
        )
        return self.call(tree, True)


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
        list_node = xml.Child().Child().Child()
        list_number = list_node.Size()
        elements = []
        for i in range(list_number):
            element_node = list_node.Child(i)
            requestID = str(element_node.Get('requestID'))
            metadatalist_node = element_node.Get('metadataList')
            metadata = parse_metadata(metadatalist_node)
            elements.append((requestID, metadata))
        return dict(elements)

    def makeCollection(self, LNs):
        tree = XMLTree(from_tree =
            ('man:makeCollection', [
                ('man:makeCollectionRequestList', [
                    ('man:makeCollectionRequestElement', [
                        ('man:requestID', 0),
                        ('man:LN', LNs[0]),
                        ('man:metadataList', [
                            ('man:metadata', [
                                ('man:section', 'states'),
                                ('man:property', 'closed'),
                                ('man:value', '0')
                            ])
                        ])
                    ])
                ])
            ])
        )
        return self.call(tree, True)

