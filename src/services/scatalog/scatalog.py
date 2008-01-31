import urlparse
import httplib
import arc
import random
from hash.xmltree import XMLTree
import traceback

catalog_uri = 'urn:scatalog'
hash_uri = 'urn:hash'

def mkuid():
    rnd = ''.join([ chr(random.randint(0,255)) for i in range(16) ])
    rnd = rnd[:6] + chr((ord(rnd[6]) & 0x4F) | 0x40) + rnd[7:8]  + chr((ord(rnd[8]) & 0xBF) | 0x80) + rnd[9:]
    uuid = 0L
    for i in range(16):
        uuid = (uuid << 8) + ord(rnd[i])
    rnd_str = '%032x' % uuid
    return '%s-%s-%s-%s-%s' % (rnd_str[0:8], rnd_str[8:12], rnd_str[12:16],  rnd_str[16:20], rnd_str[20:])

class HashClient:
    
    def __init__(self, url):
        (_, host_port, path, _, _, _) = urlparse.urlparse(url)
        if ':' in host_port:
            host, port = host_port.split(':')
        else:
            host = host_port
            port = 80
        self.hash_host = host
        self.hash_port = port
        self.hash_path = path
        self.hash_ns = arc.NS({'hash':hash_uri})

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
            lines_number = lines_node.Size()
            lines = []
            for j in range(lines_number):
                line_node = lines_node.Child(j)
                lines.append((
                    (str(line_node.Get('section')),str(line_node.Get('property'))),
                        str(line_node.Get('value'))))
            objects.append((ID,dict(lines)))
        return dict(objects)

    def change(self, changes):
        """ Call the change method of the Hash service.
        
        change(changes)

        'changes' is a list of (changeID, ID, changeType, section, property, value)
        """
        tree = XMLTree(from_tree =
            ('hash:change', [
                ('hash:changeRequestList', [
                    ('hash:changeRequestElement', [
                        ('hash:changeID', change[0]),
                        ('hash:ID', change[1]),
                        ('hash:changeType', change[2]),
                        ('hash:section', change[3]),
                        ('hash:property', change[4]),
                        ('hash:value', change[5])
                    ]) for change in changes
                ]) 
            ])
        )
        print tree
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        responses_node= xml.Child().Child().Child()
        responses_number = responses_node.Size()
        responses = []
        for i in range(responses_number):
            response_node = responses_node.Child(i)
            print response_node.GetXML()
            responses.append((str(response_node.Get('changeID')), str(response_node.Get('success'))))
        return dict(responses)

    def call(self, tree, return_tree_only = False):
        out = arc.PayloadSOAP(self.hash_ns)
        tree.add_to_node(out)
        msg = out.GetXML()
        h = httplib.HTTPConnection(self.hash_host, self.hash_port)
        h.request('POST', self.hash_path, msg)
        r = h.getresponse()
        if return_tree_only:
            resp = r.read()
            return XMLTree(from_string=resp).get_trees('///')[0]
        else:
            return r.read(), r.status, r.reason

class Catalog:
    def __init__(self, hash):
        self.hash = hash

    def newCollection(self, requests):
        response = []
        for request in requests:
            print 'Processing newCollection request:', request
            GUID = mkuid()
            changeID = 0
            changes = []
            changes.append((changeID, GUID, 'add', 'catalog', 'type', 'collection'))
            changeID += 1
            for metadata in request[1]:
                changes.append((changeID, GUID, 'add', metadata[0], metadata[1], metadata[2]))
                changeID += 1
            resp = self.hash.change(changes)
            if resp['0'] == 'added':
                success = 'created'
            else:
                success = 'failed: ' + ' '.join(resp)
            response.append((request[0], GUID, success))
        return response

    def get(self, requests):
        return self.hash.get_tree(requests)

    def traverseLN(self, requests):
        print requests
        return XMLTree()
        

class CatalogService:
    """ CatalogService class implementing the XML interface of the storage Catalog service. """

    # names of provided methods
    request_names = ['newCollection','get','traverseLN']

    def __init__(self, cfg):
        """ Constructor of the CatalogService

        CatalogService(cfg)

        'cfg' is an XMLNode which containes the config of this service.
        """
        print "CatalogService constructor called"
        # URL of the Hash
        hash_url = str(cfg.Get('HashURL'))
        hash = HashClient(hash_url)
        self.catalog = Catalog(hash)
        # set the default namespace for the Catalog service
        self.cat_ns = arc.NS({'cat':catalog_uri})

    
    def newCollection(self, inpayload):
        requests_node = inpayload.Child().Child()
        request_number = requests_node.Size()
        requests = []
        for i in range(request_number):
            request_node = requests_node.Child(i)
            rID = str(request_node.Get('requestID'))
            metadata_node = request_node.Get('metadata')
            metadata_number = metadata_node.Size()
            metadata = []
            for j in range(metadata_number):
                line_node = metadata_node.Child(j)
                line = (str(line_node.Get('section')), str(line_node.Get('property')), str(line_node.Get('value')))
                metadata.append(line)
            requests.append((rID, metadata))
        resp = self.catalog.newCollection(requests)
        tree = XMLTree(from_tree =
            ('cat:newCollectionResponseList', [
                ('cat:newCollectionResponseElement', [
                    ('cat:requestID', rID),
                    ('cat:GUID', GUID),
                    ('cat:success', success) 
                ]) for (rID, GUID, success) in resp
            ])
        )
        # create the response payload
        out = arc.PayloadSOAP(self.cat_ns)
        # create the 'newCollectionResponse' node
        response_node = out.NewChild('cat:newCollectionResponse')
        tree.add_to_node(response_node)
        return out

    def get(self, inpayload):
        requests_node = inpayload.Child().Child()
        request_number = requests_node.Size()
        requests = []
        for i in range(request_number):
            request_node = requests_node.Child(i)
            requests.append(str(request_node.Get('GUID')))
        print requests
        tree = self.catalog.get(requests)
        out = arc.PayloadSOAP(self.cat_ns)
        response_node = out.NewChild('cat:getResponse')
        tree.add_to_node(response_node)
        return out

    def traverseLN(self, inpayload):
        requests_node = inpayload.Child().Child()
        request_number = requests_node.Size()
        requests = []
        for i in range(request_number):
            request_node = requests_node.Child(i)
            requests.append((str(request_node.Get('requestID')),str(request_node.Get('LN'))))
        print dict(requests)
        tree = self.catalog.traverseLN(requests)
        out = arc.PayloadSOAP(self.cat_ns)
        response_node = out.NewChild('cat:traverseLNResponse')
        tree.add_to_node(response_node)
        return out

    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        print "Process called"
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        try:
            # gets the namespace prefix of the catalog namespace in its incoming payload
            catalog_prefix = inpayload.NamespacePrefix(catalog_uri)
            # gets the namespace prefix of the request
            request_prefix = inpayload.Child().Prefix()
            if request_prefix != catalog_prefix:
                # if the request is not in the catalog namespace
                raise Exception, 'wrong namespace (%s)' % request_prefix
            # get the name of the request without the namespace prefix
            request_name = inpayload.Child().Name()
            if request_name not in self.request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            outpayload = getattr(self,request_name)(inpayload)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            exc = traceback.format_exc()
            print exc
            # set an empty outgoing payload
            outpayload = arc.PayloadSOAP(self.cat_ns)
            outpayload.NewChild('catalog:Fault').Set(exc)
            outmsg.Payload(outpayload)
            return arc.MCC_Status(arc.STATUS_OK)
