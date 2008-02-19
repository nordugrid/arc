import urlparse
import httplib
import arc
import random
from storage.xmltree import XMLTree
import traceback
import copy

catalog_uri = 'urn:scatalog'
hash_uri = 'urn:hash'
global_root_guid= '0'

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
        print tree
        msg, status, reason = self.call(tree)
        xml = arc.XMLNode(msg)
        responses_node = xml.Child().Child().Child()
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
        for rID, metadata in requests:
            print 'Processing newCollection request:', metadata
            GUID = metadata.get(('catalog','GUID'),mkuid())
            check = self.hash.change(
                {'0': (GUID, 'add', 'catalog', 'type', 'collection', [('empty','catalog','type','')])}
            )
            if check['0'] == 'added':
                success = 'success'
                changeID = 0
                changes = []
                for ((section, property), value) in metadata.items():
                    changes.append((changeID, (GUID, 'add', section, property, value, [])))
                    changeID += 1
                changes = dict(changes)
                resp = self.hash.change(changes)
                for r in resp.keys():
                    if resp[r] != 'added':
                        success += ' (failed: %s - %s)' % (resp[r] + str(changes[r]))
            else:
                success = 'failed: entry exists'
            response.append((rID, GUID, success))
        return response

    def get(self, requests):
        return self.hash.get_tree(requests)

    def _parse_LN(self, LN):
        try:
            splitted = LN.split('/')
        except:
            raise Exception, 'Invalid Logical Name: `%s`' % LN
        guid = splitted[0]
        path = splitted[1:]
        if not guid:
            guid = global_root_guid
        return guid, path

    def _traverse(self, guid, metadata, path, traversed, GUIDs):
            try:
                if path:
                    if path[0] in ['', '.']:
                        child_guid = guid
                        child_metadata = metadata
                    else:
                        child_guid = metadata[('entries',path[0])]
                        child_metadata = self.hash.get([child_guid])[child_guid]
                    traversed.append(path.pop(0))
                    GUIDs.append(child_guid)
                    return self._traverse(child_guid, child_metadata, path, traversed, GUIDs)
                else:
                    return metadata
            except:
                print traceback.format_exc()
                return None


    def traverseLN(self, requests):
        responses = []
        for (rID, LN) in requests:
            guid, path0 = self._parse_LN(LN)
            traversed = []
            GUIDs = [guid]
            path = copy.deepcopy(path0)
            metadata0 = self.hash.get([guid])[guid]
            if not metadata0.has_key(('catalog','type')):
                response = (rID, [], False, '', None, None, LN)
            else:
                try:
                    metadata = self._traverse(guid, metadata0, path, traversed, GUIDs)
                    traversedList = [(traversed[i], GUIDs[i+1]) for i in range(len(traversed))]
                    wasComplete = len(path) == 0
                    traversedLN = '%s/%s' % (guid, '/'.join(traversed))
                    GUID = GUIDs[-1]
                    restLN = '/'.join(path)
                    response = (rID, traversedList, wasComplete, traversedLN, GUID, metadata, restLN)
                except:
                    print traceback.format_exc()
                    response = (rID, [], False, '', None, None, LN)
            responses.append(response)
        return responses

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
                line = ((str(line_node.Get('section')), str(line_node.Get('property'))), str(line_node.Get('value')))
                metadata.append(line)
            requests.append((rID, dict(metadata)))
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
        responses = self.catalog.traverseLN(requests)
        tree = XMLTree(from_tree =
            ('cat:traverseLNResponseList', [
                ('cat:traverseLNResponseElement', [
                    ('cat:requestID', rID),
                    ('cat:traversedList', [
                        ('cat:traversedListElement', [
                            ('cat:LNpart', LNpart),
                            ('cat:GUID', partGUID)
                        ]) for (LNpart, partGUID) in traversedList
                    ]),
                    ('cat:wasComplete', wasComplete and '1' or '0'),
                    ('cat:traversedLN', traversedLN),
                    ('cat:GUID', GUID),
                    ('cat:metadataList', [
                        ('cat:metadata', [
                            ('cat:section', section),
                            ('cat:property', property),
                            ('cat:value', value)
                        ]) for ((section, property), value) in metadata.items()
                    ]),
                    ('cat:restLN', restLN)
                ]) for (rID, traversedList, wasComplete, traversedLN, GUID, metadata, restLN) in responses
            ])
        )
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
