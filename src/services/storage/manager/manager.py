import urlparse
import httplib
import arc
import random
from storage.xmltree import XMLTree
from storage.client import CatalogClient
from storage.common import parse_metadata, catalog_uri, manager_uri, create_response, create_metadata, true, \
                            splitLN, remove_trailing_slash, get_child_nodes, parse_node, node_to_data, global_root_guid
import traceback

class Manager:

    def __init__(self, catalog):
        self.catalog = catalog

    def stat(self, requests):
        response = {}
        for requestID, (metadata, _, _, _, wasComplete, _) in self.catalog.traverseLN(requests).items():
            if wasComplete:
                response[requestID] = metadata
            else:
                response[requestID] = {}
        return response

    def makeCollection(self, requests):
        requests = [(rID, (remove_trailing_slash(LN), metadata)) for rID, (LN, metadata) in requests.items()]
        print '////', requests
        traverse_request = dict([(rID, LN) for rID, (LN, _) in requests])
        traverse_response = self.catalog.traverseLN(traverse_request)
        response = {}
        for rID, (LN, child_metadata) in requests:
            rootguid, _, child_name = splitLN(LN)
            print 'LN', LN, 'rootguid', rootguid, 'child_name', child_name, 'real rootguid', rootguid or global_root_guid
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[rID]
            print 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist
            if wasComplete:
                success = 'LN exists'
            elif restLN != child_name:
                success = 'parent does not exist'
            elif child_name == '': # this only can happen if the LN was a single GUID
                child_metadata[('catalog','type')] = 'collection'
                child_metadata[('catalog','GUID')] = rootguid or global_root_guid
                print "nc_resp = self.catalog.new({'0' : child_metadata})", child_metadata
                nc_resp = self.catalog.new({'0' : child_metadata})
                (child_GUID, nc_succ) = nc_resp['0']
                if nc_succ != 'success':
                    success = 'failed to create new collection'
                else:
                    success = 'done'
            else:
                child_metadata[('catalog','type')] = 'collection'
                nc_resp = self.catalog.new({'0' : child_metadata})
                (child_GUID, nc_succ) = nc_resp['0']
                if nc_succ != 'success':
                    success = 'failed to create new collection'
                else:
                    print 'adding', child_GUID, 'to parent', GUID
                    mm_resp = self.catalog.modifyMetadata({'0' : (GUID, 'add', 'entries', child_name, child_GUID)})
                    print 'modifyMetadata response', mm_resp
                    mm_succ = mm_resp['0']
                    if mm_succ != 'set':
                        self.catalog.remove({'0' : child_GUID})
                        success = 'failed adding child to parent'
                    else:
                        success = 'done'
            response[rID] = success
        return response

    def list(self, requests, neededMetadata):
        traverse_response = self.catalog.traverseLN(requests)
        response = {}
        for requestID, LN in requests.items():
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[requestID]
            if wasComplete:
                GUIDs = dict([(name, GUID)
                    for (section, name), GUID in metadata.items() if section == 'entries'])
                metadata = self.catalog.get(GUIDs.values(), neededMetadata)
                entries = dict([(name, (GUID, metadata[GUID])) for name, GUID in GUIDs.items()])
            else:
                entries = {}
            response[requestID] = entries
        return response

    def move(self, requests):
        traverse_request = {}
        for requestID, (sourceLN, targetLN, _) in requests.items():
            traverse_request[requestID + 'source'] = sourceLN
            traverse_request[requestID + 'target'] = targetLN
        traverse_response = self.catalog.traverseLN(traverse_request)
        print '\/\/', traverse_response
        response = {}
        for requestID, (sourceLN, targetLN, preserveOriginal) in requests.items():
            print requestID, sourceLN, targetLN, preserveOriginal
            _, _, old_child_name = splitLN(sourceLN)
            _, _, new_child_name = splitLN(targetLN)
            _, sourceGUID, _, _, sourceWasComplete, sourceTraversedList \
                = traverse_response[requestID + 'source']
            _, targetGUID, _, targetRestLN, targetWasComplete, _ \
                = traverse_response[requestID + 'target']
            if not sourceWasComplete:
                success = 'nosuchLN'
            elif targetWasComplete and new_child_name != '':
                success = 'targetexists'
            elif not targetWasComplete and targetRestLN != new_child_name:
                success = 'invalidtarget'
            else: 
                if new_child_name == '':
                    new_child_name = old_child_name
                print 'adding', sourceGUID, 'to parent', targetGUID
                mm_resp = self.catalog.modifyMetadata(
                    {'0' : (targetGUID, 'add', 'entries', new_child_name, sourceGUID)})
                mm_succ = mm_resp['0']
                if mm_succ != 'set':
                    success = 'failed adding child to parent'
                else:
                    if preserveOriginal == true:
                        success = 'moved'
                    else:
                        source_parent_guid = sourceTraversedList[-2][1]
                        print 'removing', sourceGUID, 'from parent', source_parent_guid
                        mm_resp = self.catalog.modifyMetadata(
                            {'0' : (source_parent_guid, 'unset', 'entries', old_child_name, '')})
                        mm_succ = mm_resp['0']
                        if mm_succ != 'unset':
                            success = 'failed removing child from parent'
                        else:
                            success = 'moved'
            response[requestID] = success
        return response

class ManagerService:

    def __init__(self, cfg):
        print "Storage Manager service constructor called"
        catalog_url = str(cfg.Get('CatalogURL'))
        catalog = CatalogClient(catalog_url)
        self.manager = Manager(catalog)
        self.man_ns = arc.NS({'man':manager_uri})

    def stat(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (str(request_node.Get('requestID')), str(request_node.Get('LN')))
                for request_node in request_nodes
        ])
        response = self.manager.stat(requests)
        for requestID, metadata in response.items():
            response[requestID] = create_metadata(metadata, 'man')
        return create_response('man:stat',
            ['man:requestID', 'man:metadataList'], response, self.man_ns, single = True)

    def makeCollection(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (str(request_node.Get('requestID')), 
                (str(request_node.Get('LN')), parse_metadata(request_node.Get('metadataList')))
            ) for request_node in request_nodes
        ])
        response = self.manager.makeCollection(requests)
        return create_response('man:makeCollection',
            ['man:requestID', 'man:success'], response, self.man_ns, single = True)

    def list(self, inpayload):
        requests = parse_node(inpayload.Child().Get('listRequestList'),
            ['requestID', 'LN'], single = True)
        neededMetadata = [
            node_to_data(node, ['section', 'property'], single = True)
                for node in get_child_nodes(inpayload.Child().Get('neededMetadataList'))
        ]
        response0 = self.manager.list(requests, neededMetadata)
        response = dict([
            (requestID,
            [('man:entry', [
                ('man:name', name),
                ('man:GUID', GUID),
                ('man:metadataList', create_metadata(metadata))
            ]) for name, (GUID, metadata) in entries.items()]
        ) for requestID, entries in response0.items()])
        return create_response('man:list',
            ['man:requestID', 'man:entries'], response, self.man_ns, single = True)

    def move(self, inpayload):
        requests = parse_node(inpayload.Child().Child(),
            ['requestID', 'sourceLN', 'targetLN', 'preserveOriginal'])
        response = self.manager.move(requests)
        return create_response('man:move',
            ['man:requestID', 'man:status'], response, self.man_ns, single = True)


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

    # names of provided methods
    request_names = ['stat', 'makeCollection', 'list', 'move']
