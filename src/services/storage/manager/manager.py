import urlparse
import httplib
import arc
import random
from storage.xmltree import XMLTree
from storage.client import CatalogClient, ElementClient
from storage.common import parse_metadata, catalog_uri, manager_uri, create_response, create_metadata, true, \
                            splitLN, remove_trailing_slash, get_child_nodes, parse_node, node_to_data, global_root_guid, \
                            serialize_ids, parse_ids
import traceback

class Manager:

    def __init__(self, catalog, element):
        self.catalog = catalog
        self.element = element

    def stat(self, requests):
        response = {}
        for requestID, (metadata, _, _, _, wasComplete, _) in self.catalog.traverseLN(requests).items():
            if wasComplete:
                response[requestID] = metadata
            else:
                response[requestID] = {}
        return response
    
    def _traverse(self, requests):
        requests = [(rID, [remove_trailing_slash(data[0])] + list(data[1:])) for rID, data in requests.items()]
        print '//// _traverse request trailing slash removed:', dict(requests)
        traverse_request = dict([(rID, data[0]) for rID, data in requests])
        traverse_response = self.catalog.traverseLN(traverse_request)
        return requests, traverse_response

    def _new(self, child_metadata, child_name = None, parent_GUID = None):
        try:
            new_response = self.catalog.new({'_new' : child_metadata})
            (child_GUID, new_success) = new_response['_new']
            if new_success != 'success':
                return 'failed to create new catalog entry', child_GUID
            else:
                if child_name and parent_GUID:
                    print 'adding', child_GUID, 'to parent', parent_GUID
                    modify_response = self.catalog.modifyMetadata({'_new' : (parent_GUID, 'add', 'entries', child_name, child_GUID)})
                    print 'modifyMetadata response', modify_response
                    modify_success = modify_response['_new']
                    if modify_success != 'set':
                        print 'modifyMetadata failed, removing the new catalog entry', child_GUID
                        self.catalog.remove({'_new' : child_GUID})
                        return 'failed to add child to parent', child_GUID
                    else:
                        return 'done', child_GUID
                else: # no parent given, skip the 'adding child to parent' part
                    return 'done', child_GUID
        except:
            print traceback.format_exc()
            return 'internal error', None

    def getFile(self, requests):
        requests, traverse_response = self._traverse(requests)
        response = {}
        for rID, (LN, protocols) in requests:
            turl = ''
            protocol = ''
            try:
                print traverse_response[rID]
                metadata, GUID, traversedLN, restLN, wasComplete, traversedList = traverse_response[rID]
                if not wasComplete:
                    success = 'not found'
                else:
                    valid_locations = [parse_ids(location) + [state] for (section, location), state in metadata.items() if section == 'locations' and state == 'alive']
                    if not valid_locations:
                        success = 'file has no valid replica'
                    else:
                        location = random.choice(valid_locations)
                        print 'location chosen:', location
                        url, referenceID, _ = location
                        get_response = dict(ElementClient(url).get({'getFile' : [('referenceID', referenceID), ('protocol', 'byteio')]})['getFile'])
                        if get_response.has_key('error'):
                            print 'ERROR', get_response['error']
                            success = 'error while getting TURL (%s)' % get_response['error']
                        else:
                            turl = get_response['TURL']
                            protocol = get_response['protocol']
                            success = 'done'
            except:
                success = 'internal error (%s)' % traceback.format_exc()
            response[rID] = (success, turl, protocol)
        return response 

    def putFile(self, requests):
        requests, traverse_response = self._traverse(requests)
        response = {}
        for rID, (LN, child_metadata, protocols) in requests:
            turl = ''
            protocol = ''
            metadata_ok = False
            try:
                print protocols
                size = child_metadata[('states', 'size')]
                checksum = child_metadata[('states', 'checksum')]
                checksumType = child_metadata[('states', 'checksumType')]
                metadata_ok = True
            except Exception, e:
                success = 'missing metadata ' + str(e)
            if metadata_ok:
                try:
                    rootguid, _, child_name = splitLN(LN)
                    print 'LN', LN, 'rootguid', rootguid, 'child_name', child_name, 'real rootguid', rootguid or global_root_guid
                    metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[rID]
                    print 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist
                    if wasComplete:
                        success = 'LN exists'
                    elif child_name == '': # this only can happen if the LN was a single GUID
                        child_metadata[('catalog','type')] = 'file'
                        child_metadata[('catalog','GUID')] = rootguid or global_root_guid
                        success, GUID = self._new(child_metadata)
                    elif restLN != child_name or GUID == '':
                        success = 'parent does not exist'
                    else:
                        child_metadata[('catalog','type')] = 'file'
                        success, GUID = self._new(child_metadata, child_name, GUID)
                    if success == 'done':
                        put_request = [('size', size), ('checksumType', checksumType),
                            ('checksum', checksum), ('GUID', GUID)] + \
                            [('protocol', protocol) for protocol in protocols]
                        put_response = dict(self.element.put({'putFile': put_request})['putFile'])
                        if put_response.has_key('error'):
                            print 'ERROR', put_response['error']
                            # we should handle this, remove the new file or something
                            success = 'put error (%s)' % put_response['error']
                        else:
                            turl = put_response['TURL']
                            protocol = put_response['protocol']
                            referenceID = put_response['referenceID']
                            serviceID = self.element.url
                            print 'serviceID', serviceID, 'referenceID:', referenceID, 'turl', turl, 'protocol', protocol
                            modify_response = self.catalog.modifyMetadata({'putFile' :
                                    (GUID, 'set', 'locations', serialize_ids([serviceID, referenceID]), 'creating')})
                            modify_success = modify_response['putFile']
                            if modify_success != 'set':
                                print 'failed to add location to file', GUID, LN, serviceID, referenceID
                                success = 'failed to add new location to file'
                                # need some handling
                            else:
                                success = 'done'
                except:
                    success = 'internal error (%s)' % traceback.format_exc()
            response[rID] = (success, turl, protocol)
        return response

    def makeCollection(self, requests):
        requests, traverse_response = self._traverse(requests)
        response = {}
        for rID, (LN, child_metadata) in requests:
            rootguid, _, child_name = splitLN(LN)
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[rID]
            print 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist
            if wasComplete:
                success = 'LN exists'
            elif child_name == '': # this only can happen if the LN was a single GUID
                child_metadata[('catalog','type')] = 'collection'
                child_metadata[('catalog','GUID')] = rootguid or global_root_guid
                success, _ = self._new(child_metadata)
            elif restLN != child_name or GUID == '':
                success = 'parent does not exist'
            else:
                child_metadata[('catalog','type')] = 'collection'
                success, _ = self._new(child_metadata, child_name, GUID)
            response[rID] = success
        return response

    def list(self, requests, neededMetadata):
        traverse_response = self.catalog.traverseLN(requests)
        response = {}
        for requestID, LN in requests.items():
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[requestID]
            if wasComplete:
                type = metadata[('catalog', 'type')]
                if type == 'file':
                    status = 'is a file'
                    entries = {}
                else:
                    status = 'found'
                    GUIDs = dict([(name, GUID)
                        for (section, name), GUID in metadata.items() if section == 'entries'])
                    metadata = self.catalog.get(GUIDs.values(), neededMetadata)
                    entries = dict([(name, (GUID, metadata[GUID])) for name, GUID in GUIDs.items()])
            else:
                entries = {}
                status = 'not found'
            response[requestID] = (entries, status)
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
                    {'move' : (targetGUID, 'add', 'entries', new_child_name, sourceGUID)})
                mm_succ = mm_resp['move']
                if mm_succ != 'set':
                    success = 'failed adding child to parent'
                else:
                    if preserveOriginal == true:
                        success = 'moved'
                    else:
                        source_parent_guid = sourceTraversedList[-2][1]
                        print 'removing', sourceGUID, 'from parent', source_parent_guid
                        mm_resp = self.catalog.modifyMetadata(
                            {'move' : (source_parent_guid, 'unset', 'entries', old_child_name, '')})
                        mm_succ = mm_resp['move']
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
        element_url = str(cfg.Get('ElementURL'))
        element = ElementClient(element_url)
        self.manager = Manager(catalog, element)
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

    def getFile(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                (
                    str(request_node.Get('LN')),
                    [str(node) for node in request_node.XPathLookup('//man:protocol', self.man_ns)]
                )
            ) for request_node in request_nodes
        ])
        response = self.manager.getFile(requests)
        return create_response('man:getFile',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.man_ns)
    
    def putFile(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                (
                    str(request_node.Get('LN')),
                    parse_metadata(request_node.Get('metadataList')),
                    [str(node) for node in request_node.XPathLookup('//man:protocol', self.man_ns)]
                )
            ) for request_node in request_nodes
        ])
        response = self.manager.putFile(requests)
        return create_response('man:putFile',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.man_ns)

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
            ([('man:entry', [
                ('man:name', name),
                ('man:GUID', GUID),
                ('man:metadataList', create_metadata(metadata))
            ]) for name, (GUID, metadata) in entries.items()],
            status)
        ) for requestID, (entries, status) in response0.items()])
        return create_response('man:list',
            ['man:requestID', 'man:entries', 'man:status'], response, self.man_ns)

    def move(self, inpayload):
        requests = parse_node(inpayload.Child().Child(),
            ['requestID', 'sourceLN', 'targetLN', 'preserveOriginal'])
        response = self.manager.move(requests)
        return create_response('man:move',
            ['man:requestID', 'man:status'], response, self.man_ns, single = True)


    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
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
            print '     manager.%s called' % request_name
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
    request_names = ['stat', 'makeCollection', 'list', 'move', 'putFile', 'getFile']
