from storage.common import hash_uri, catalog_uri, manager_uri, parse_metadata
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
            return XMLTree(from_string=resp).get_trees('///')[0]
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
            hash_prefix + ':lines' : 'cat:metadataList',
            hash_prefix + ':line' : 'cat:metadata',
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
            lines_node = obj_node.Get('lines')
            lines = parse_metadata(lines_node)
            objects.append((ID,lines))
        return dict(objects)

    def change(self, changes):
        """ Call the change method of the Hash service.
        
        change(changes)

        'changes' is a dictionary of {changeID : (ID, changeType, section, property, value, conditions)}
            where 'conditions' is a list of (conditionType, section, property, value)
        """
        tree = XMLTree(from_tree =
            ('hash:change', [
                ('hash:changeRequestList', [
                    ('hash:changeRequestElement', [
                        ('hash:changeID', chID),
                        ('hash:ID', change[0]),
                        ('hash:changeType', change[1]),
                        ('hash:section', change[2]),
                        ('hash:property', change[3]),
                        ('hash:value', change[4]),
                        ('hash:conditionList', [
                            ('hash:condition', [
                                ('hash:conditionType',condition[0]),
                                ('hash:section',condition[1]),
                                ('hash:property',condition[2]),
                                ('hash:value',condition[3])
                            ]) for condition in change[5]
                        ])
                    ]) for chID, change in changes.items()
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
            responses.append((str(response_node.Get('changeID')), str(response_node.Get('success'))))
        return dict(responses)

class CatalogClient(Client):
    
    def __init__(self, url):
        ns = arc.NS({'cat':catalog_uri})
        Client.__init__(self, url, ns)

    def get(self, IDs):
        tree = XMLTree(from_tree =
            ('cat:get', [
                ('cat:getRequestList', [
                    ('cat:getRequestElement', [
                        ('cat:GUID', i)
                    ]) for i in IDs
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
                    ]) for rID, LN in requests
                ])
            ])
        )
        responses = self.call(tree, True)
        return dict([(response['requestID'], response) for response in responses.get_dicts('///')])

    def newCollection(self, requests):
        tree = XMLTree(from_tree =
            ('cat:newCollection', [
                ('cat:newCollectionRequestList', [
                    ('cat:newCollectionRequestElement', [
                        ('cat:requestID', rID),
                        ('cat:metadataList', [
                            ('cat:metadataList', [
                                ('cat:metadata', [
                                    ('cat:section', section),
                                    ('cat:property', property),
                                    ('cat:value', value)
                                ]) for ((section, property), value) in metadataList
                            ])
                        ])
                    ]) for rID, metadataList in requests
                ])
            ])
        )
        return self.call(tree, True)


