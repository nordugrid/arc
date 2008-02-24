import urlparse
import httplib
import arc
import random
from storage.xmltree import XMLTree
from storage.client import HashClient
from storage.common import catalog_uri, global_root_guid, mkuid, parse_metadata, true, false
import traceback
import copy

class Catalog:
    def __init__(self, hash):
        self.hash = hash

    def newCollection(self, requests):
        response = []
        for rID, metadata in requests:
            print 'Processing newCollection request:', metadata
            GUID = metadata.get(('catalog','GUID'), mkuid())
            check = self.hash.change(
                {'0': (GUID, 'set', 'catalog', 'type', 'collection', {'0' : ('unset','catalog','type','')})}
            )
            status, failedCondition = check['0']
            if status == 'set':
                success = 'success'
                changeID = 0
                changes = []
                for ((section, property), value) in metadata.items():
                    changes.append((changeID, (GUID, 'set', section, property, value, {})))
                    changeID += 1
                changes = dict(changes)
                resp = self.hash.change(changes)
                for r in resp.keys():
                    if resp[r][0] != 'set':
                        success += ' (failed: %s - %s)' % (resp[r][0] + str(changes[r]))
            elif failedCondition == '0':
                success = 'failed: entry exists'
            else:
                success = 'failed: ' + status
            response.append((rID, (GUID, success)))
        return dict(response)

    def get(self, requests):
        return self.hash.get_tree(requests)

    def _parse_LN(self, LN):
        try:
            splitted = LN.split('/')
        except:
            raise Exception, 'Invalid Logical Name: `%s`' % LN
        guid = splitted[0]
        path = splitted[1:]
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
            except KeyError:
                return {}
            except:
                print traceback.format_exc()
                return {}


    def traverseLN(self, requests):
        responses = []
        for (rID, LN) in requests:
            guid0, path0 = self._parse_LN(LN)
            if not guid0:
                guid = global_root_guid
            else:
                guid = guid0
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
                    traversedLN = guid0 + '/' + '/'.join(traversed)
                    GUID = GUIDs[-1]
                    restLN = '/'.join(path)
                    response = rID, (traversedList, wasComplete, traversedLN, GUID, metadata, restLN)
                except:
                    print traceback.format_exc()
                    response = rID, ([], False, '', None, None, LN)
            responses.append(response)
        return dict(responses)

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
            metadatalist_node = request_node.Get('metadataList')
            metadata = parse_metadata(metadatalist_node)
            requests.append((rID, metadata))
        requests = dict(requests)
        resp = self.catalog.newCollection(requests)
        tree = XMLTree(from_tree =
            ('cat:newCollectionResponseList', [
                ('cat:newCollectionResponseElement', [
                    ('cat:requestID', rID),
                    ('cat:GUID', GUID),
                    ('cat:success', success) 
                ]) for rID, (GUID, success) in resp.items()
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
        request = dict(requests)
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
                    ('cat:wasComplete', wasComplete and true or false),
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
                ]) for rID, (traversedList, wasComplete, traversedLN, GUID, metadata, restLN) in responses.items()
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
