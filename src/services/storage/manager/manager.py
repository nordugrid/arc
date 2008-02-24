import urlparse
import httplib
import arc
import random
from storage.xmltree import XMLTree
from storage.client import CatalogClient
from storage.common import parse_metadata, catalog_uri, manager_uri, \
                            basename, dirname, remove_trailing_slash, true, false
import traceback

class Manager:

    def __init__(self, catalog):
        self.catalog = catalog

    def stat(self, requests):
        responses = []
        for response in self.catalog.traverseLN(requests).values():
            if response['wasComplete'] == false:
                responses.append((response['requestID'],[]))
            else:
                responses.append((response['requestID'],
                    [dict(metadata[1]) for metadata in response['metadataList']]))
        return responses

    def makeCollection(self, requests):
        requests = [(rID, (remove_trailing_slash(LN), metadata)) for rID, (LN, metadata) in requests]
        print requests
        traverse_request = [(rID, LN) for rID, (LN, _) in requests]
        print traverse_request
        traverse_response = self.catalog.traverseLN(traverse_request)
        print traverse_response
        responses = []
        for rID, (LN, metadata) in requests:
            trav = traverse_response[rID]
            if trav['restLN'] != basename(LN):
                if trav['wasComplete'] == true:
                    success = 'LN exists'
                else:
                    success = 'parent does not exist'
            else:
                print 'OK!', trav
                nc_resp = self.catalog.newCollection([('0', metadata)])
                nc_dict = nc_resp.get_dict('///')
                if nc_dict['success'] != 'success':
                    success = 'failed to create new collection'
                else:
                    
                    success = 'OK'
            responses.append((rID, success))
        return responses

class ManagerService:

    # names of provided methods
    request_names = ['stat','makeCollection']

    def __init__(self, cfg):
        print "Storage Manager service constructor called"
        catalog_url = str(cfg.Get('CatalogURL'))
        catalog = CatalogClient(catalog_url)
        self.manager = Manager(catalog)
        self.man_ns = arc.NS({'man':manager_uri})
    
    def stat(self, inpayload):
        requests_node = inpayload.Child().Child()
        request_number = requests_node.Size()
        requests = []
        for i in range(request_number):
            request_node = requests_node.Child(i)
            requests.append((str(request_node.Get('requestID')),str(request_node.Get('LN'))))
        responses = self.manager.stat(requests)
        out = arc.PayloadSOAP(self.man_ns)
        response_node = out.NewChild('man:statResponse')
        tree = XMLTree(from_tree =
            ('man:statResponseList', [
                ('man:statResponseElement', [
                    ('man:requestID', rID),
                    ('man:metadataList', [
                        ('man:metadata', [
                            ('man:section', metadata['section']),
                            ('man:property', metadata['property']),
                            ('man:value', metadata['value'])
                        ]) for metadata in metadataList
                    ])
                ]) for rID, metadataList in responses
            ])
        )
        tree.add_to_node(response_node)
        return out

    def makeCollection(self, inpayload):
        requests_node = inpayload.Child().Child()
        request_number = requests_node.Size()
        requests = []
        for i in range(request_number):
            request_node = requests_node.Child(i)
            metadatalist_node = request_node.Get('metadataList')
            metadata = parse_metadata(metadatalist_node)
            requests.append((str(request_node.Get('requestID')),(str(request_node.Get('LN')),metadata)))
        responses = self.manager.makeCollection(requests)
        out = arc.PayloadSOAP(self.man_ns)
        response_node = out.NewChild('man:makeCollectionResponse')
        tree = XMLTree(from_tree =
            ('man:makeCollectionResponseList', [
                ('man:makeCollectionResponseElement', [
                    ('man:requestID', rID),
                    ('man:success', success)
                ]) for rID, success in responses
            ])
        )
        tree.add_to_node(response_node)
        return out

    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        print "Process called"
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        try:
            # gets the namespace prefix of the manager namespace in its incoming payload
            manager_prefix = inpayload.NamespacePrefix(manager_uri)
            # gets the namespace prefix of the request
            request_prefix = inpayload.Child().Prefix()
            if request_prefix != manager_prefix:
                # if the request is not in the manager namespace
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
            outpayload = arc.PayloadSOAP(self.man_ns)
            outpayload.NewChild('manager:Fault').Set(exc)
            outmsg.Payload(outpayload)
            return arc.MCC_Status(arc.STATUS_OK)
