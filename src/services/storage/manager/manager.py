import urlparse
import httplib
import arc
import random
import time
from storage.xmltree import XMLTree
from storage.client import CatalogClient, ElementClient
from storage.common import parse_metadata, catalog_uri, manager_uri, create_response, create_metadata, true, \
                            splitLN, remove_trailing_slash, get_child_nodes, parse_node, node_to_data, global_root_guid, \
                            serialize_ids, parse_ids, sestore_guid
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
            success = 'unknown'
            try:
                print traverse_response[rID]
                metadata, GUID, traversedLN, restLN, wasComplete, traversedList = traverse_response[rID]
                if not wasComplete:
                    success = 'not found'
                else:
                    type = metadata[('catalog', 'type')]
                    if type != 'file':
                        success = 'is not a file'
                    else:
                        valid_locations = [parse_ids(location) + [state] for (section, location), state in metadata.items() if section == 'locations' and state == 'alive']
                        if not valid_locations:
                            success = 'file has no valid replica'
                        else:
                            ok = False
                            while not ok and len(valid_locations) > 0:
                                location = valid_locations.pop(random.choice(range(len(valid_locations))))
                                print 'location chosen:', location
                                url, referenceID, _ = location
                                get_response = dict(ElementClient(url).get({'getFile' :
                                    [('referenceID', referenceID), ('protocol', 'byteio')]})['getFile'])
                                if get_response.has_key('error'):
                                    print 'ERROR', get_response['error']
                                    success = 'error while getting TURL (%s)' % get_response['error']
                                else:
                                    turl = get_response['TURL']
                                    protocol = get_response['protocol']
                                    success = 'done'
                                    ok = True
            except:
                success = 'internal error (%s)' % traceback.format_exc()
            response[rID] = (success, turl, protocol)
        return response
    
    def addReplica(self, requests, protocols):
        data = self.catalog.get(requests.values(), [('states','')])
        response = {}
        for rID, GUID in requests.items():
            states = data[GUID]
            print 'addReplica', 'requestID', rID, 'GUID', GUID, 'states', states, 'protocols', protocols
            size = states[('states','size')]
            checksumType = states[('states','checksumType')]
            checksum = states[('states','checksum')]
            success, turl, protocol = self._add_replica(size, checksumType, checksum, GUID, protocols)
            response[rID] = (success, turl, protocol)
        return response

    def find_alive_se(self):
        SEs = self.catalog.get([sestore_guid])[sestore_guid]
        print 'Registered Storage Elements in Catalog', SEs
        alive_SEs = [s for (s, p), v in SEs.items() if p == 'nextHeartbeat' and int(v) > 0]
        print 'Alive Storage Elements:', alive_SEs
        try:
            se = random.choice(alive_SEs)
            print 'Storage Element chosen:', se        
            return ElementClient(se)
        except:
            traceback.print_exc()
            return None

    def _add_replica(self, size, checksumType, checksum, GUID, protocols):
        turl = ''
        protocol = ''
        put_request = [('size', size), ('checksumType', checksumType),
            ('checksum', checksum), ('GUID', GUID)] + \
            [('protocol', protocol) for protocol in protocols]
        element = self.find_alive_se()
        if not element:
            return 'no storage element found', turl, protocol
        put_response = dict(element.put({'putFile': put_request})['putFile'])
        if put_response.has_key('error'):
            print 'ERROR', put_response['error']
            # we should handle this, remove the new file or something
            return 'put error (%s)' % put_response['error'], turl, protocols
        else:
            turl = put_response['TURL']
            protocol = put_response['protocol']
            referenceID = put_response['referenceID']
            serviceID = element.url
            print 'serviceID', serviceID, 'referenceID:', referenceID, 'turl', turl, 'protocol', protocol
            modify_response = self.catalog.modifyMetadata({'putFile' :
                    (GUID, 'set', 'locations', serialize_ids([serviceID, referenceID]), 'creating')})
            modify_success = modify_response['putFile']
            if modify_success != 'set':
                print 'failed to add location to file', 'GUID', GUID, 'serviceID', serviceID, 'referenceID', referenceID
                return 'failed to add new location to file', turl, protocol
                # need some handling
            else:
                return 'done', turl, protocol
            
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
                        success, turl, protocol = self._add_replica(size, checksumType, checksum, GUID, protocols)
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

from storage.service import Service

class ManagerService(Service):

    def __init__(self, cfg):
        # names of provided methods
        request_names = ['stat', 'makeCollection', 'list', 'move', 'putFile', 'getFile', 'addReplica']
        # call the Service's constructor
        Service.__init__(self, 'Manager', request_names, 'man', manager_uri)
        catalog_url = str(cfg.Get('CatalogURL'))
        catalog = CatalogClient(catalog_url)
        element_url = str(cfg.Get('ElementURL'))
        element = ElementClient(element_url)
        self.manager = Manager(catalog, element)

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
            ['man:requestID', 'man:metadataList'], response, self.newSOAPPayload(), single = True)

    def getFile(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                (
                    str(request_node.Get('LN')),
                    [str(node) for node in request_node.XPathLookup('//man:protocol', self.ns)]
                )
            ) for request_node in request_nodes
        ])
        response = self.manager.getFile(requests)
        return create_response('man:getFile',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.newSOAPPayload())
    
    def addReplica(self, inpayload):
        protocols = [str(node) for node in inpayload.XPathLookup('//man:protocol', self.newSOAPPayload())]
        request_nodes = get_child_nodes(inpayload.Child().Get('addReplicaRequestList'))
        requests = dict([(str(request_node.Get('requestID')), str(request_node.Get('GUID')))
                for request_node in request_nodes])
        response = self.manager.addReplica(requests, protocols)
        return create_response('man:addReplica',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.newSOAPPayload())
    
    def putFile(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                (
                    str(request_node.Get('LN')),
                    parse_metadata(request_node.Get('metadataList')),
                    [str(node) for node in request_node.XPathLookup('//man:protocol', self.newSOAPPayload())]
                )
            ) for request_node in request_nodes
        ])
        response = self.manager.putFile(requests)
        return create_response('man:putFile',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.newSOAPPayload())

    def makeCollection(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (str(request_node.Get('requestID')), 
                (str(request_node.Get('LN')), parse_metadata(request_node.Get('metadataList')))
            ) for request_node in request_nodes
        ])
        response = self.manager.makeCollection(requests)
        return create_response('man:makeCollection',
            ['man:requestID', 'man:success'], response, self.newSOAPPayload(), single = True)

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
            ['man:requestID', 'man:entries', 'man:status'], response, self.newSOAPPayload())

    def move(self, inpayload):
        requests = parse_node(inpayload.Child().Child(),
            ['requestID', 'sourceLN', 'targetLN', 'preserveOriginal'])
        response = self.manager.move(requests)
        return create_response('man:move',
            ['man:requestID', 'man:status'], response, self.newSOAPPayload(), single = True)

