import urlparse
import httplib
import arc
import random
import time
from storage.xmltree import XMLTree
from storage.client import CatalogClient, ElementClient
from storage.common import parse_metadata, catalog_uri, manager_uri, create_response, create_metadata, true, \
                            splitLN, remove_trailing_slash, get_child_nodes, parse_node, node_to_data, global_root_guid, \
                            serialize_ids, deserialize_ids, sestore_guid
import traceback

class Manager:

    def __init__(self, catalog):
        """ Constructor of the Manager business-logic class.
        
        Manager(catalog)
        
        catalog is CatalogClient object which can be used to access a Catalog service
        """
        self.catalog = catalog

    def stat(self, requests):
        """ Returns stat information about entries.
        
        stat(requests)
        
        requests is a dictionary with requestIDs as keys, and Logical Names as values.
    
        Returns a dictionary with requestIDs as keys, and metadata as values.
        The 'metadata' is a dictionary with (section, property) pairs as keys.
        """
        response = {}
        # get the information from the catalog
        traverse_response = self.catalog.traverseLN(requests)
        # we are only interested in the metadata and if the traversing was complete or not
        for requestID, (metadata, _, _, _, wasComplete, _) in traverse_response.items():
            if wasComplete: # if it was complete, then we found the entry and got the metadata
                response[requestID] = metadata
            else: # if it was not complete, then we didn't found the entry, so metadata will be empty
                response[requestID] = {}
        return response
    
    def delFile(self, requests):
        """ Delete a file from the storage: initiate the process.
        
        delFile(requests)
        
        requests is a dictionary with requestID as key and (Logical Name, child metadata, protocols) as value
        """
        import time
        response = {}
        # get the information from the catalog
        #requests, traverse_response = self._traverse(requests)
        traverse_response = self.catalog.traverseLN(requests)
        cat_rem_requests = {}
        cat_mod_requests = {}
        check_again = []
        for requestID, (metadata, GUID, LN, _, wasComplete, traversedList) in traverse_response.items():
            if wasComplete and metadata[('catalog', 'type')]=='file': # if it was complete, then we found the entry and got the metadata
                parentno = len([property for (section, property), value in metadata.items() if section == 'parents'])
                if parentno < 2:
                    # remove the file itself,  if this file has only one parent (hardlink), or has no parent at all
                    cat_rem_requests[requestID] = GUID
                # if this entry has a parent:
                if len(traversedList)>1:
                    # notify the parents
                    parentLN, parentGUID = traversedList[-2]
                    cat_mod_requests[requestID + '_1'] = (parentGUID, 'unset', 'entries',
                                                      traversedList[-1][0],GUID)
                                                    
                    if parentno > 1:
                        # remove the parent entry from the child, if this file has more than one parents (hardlinks)
                        _, _, filename = splitLN(LN)
                        cat_mod_requests[requestID + '_2'] = (GUID, 'unset', 'parents', '%s/%s' % (parentGUID, filename), '')
                        # if the other parents of this file are removing it at the same time, all are thinking that there are more parents
                        check_again.append(GUID)
                response[requestID] = 'deleted'
            else: # if it was not complete, then we didn't find the entry, so metadata will be empty
                response[requestID] = 'nosuchLN'
        success = self.catalog.remove(cat_rem_requests)
        modify_success = self.catalog.modifyMetadata(cat_mod_requests)
        # check the files with more parents (hardlinks) again
        if check_again:
            self.log('DEBUG', '\n\n!!!!check_again', check_again)
            checked_again = self.catalog.get(check_again)
            do_delete = {}
            for GUID, metadata in checked_again.items():
                # if a file has no parents now, remove it
                parentno = len([property for (section, property), value in metadata.items() if section == 'parents'])
                if parentno == 0:
                    do_delete[GUID] = GUID
            self.log('DEBUG', '\n\n!!!!do_delete', do_delete)
            self.catalog.remove(do_delete)
        return response
            

    def _traverse(self, requests):
        """ Helper method which connects the catalog, and traverses the LNs of the requests.
        
        _traverse(requests)
        
        Removes the trailing slash from  all the LNs in the request.
        Returns the requests and the traverse response.
        """
        # in each request the requestID is the key and the value is a list
        # the first item of the list is the Logical Name, we want to remove the trailing slash, and
        # leave the other items intact
        requests = [(rID, [remove_trailing_slash(data[0])] + list(data[1:])) for rID, data in requests.items()]
        self.log('DEBUG', '//// _traverse request trailing slash removed:', dict(requests))
        # then we do the traversing. a traverse request contains a requestID and the Logical Name
        # so we just need the key (which is the request ID) and the first item of the value (which is the LN)
        traverse_request = dict([(rID, data[0]) for rID, data in requests])
        # call the catalog service
        traverse_response = self.catalog.traverseLN(traverse_request)
        # return the requests as list (without the trailing slashes) and the traverse response from the catalog
        return requests, traverse_response

    def _new(self, child_metadata, child_name = None, parent_GUID = None):
        """ Helper method which create a new entry in the catalog.
        
        _new(child_metadata, child_name = None, parent_GUID = None)
        
        child_metadata is a dictionary with {(section, property) : values} containing the metadata of the new entry
        child_name is the name of the new entry 
        parent_GUID is the GUID of the parent of the new entry
        
        This method creates a new catalog-entry with the given metadata.
        If child_name and parent_GUID are both given, then this method adds a new entry to the parent collection.
        """
        try:
            # set creation time stamp
            child_metadata[('timestamps', 'created')] = str(time.time())
            if child_name and parent_GUID:
                child_metadata[('parents', '%s/%s' % (parent_GUID, child_name))] = 'parent'
            # call the new method of the catalog with the child's metadata (requestID is '_new')
            new_response = self.catalog.new({'_new' : child_metadata})
            # we can access the response with the requestID, so we get the GUID of the newly created entry
            (child_GUID, new_success) = new_response['_new']
            # if the new method was not successful
            if new_success != 'success':
                return 'failed to create new catalog entry', child_GUID
            else:
                # if it was successful and we have a parent collection
                if child_name and parent_GUID:
                    # we need to add the newly created catalog-entry to the parent collection
                    self.log('DEBUG', 'adding', child_GUID, 'to parent', parent_GUID)
                    # this modifyMetadata request adds a new (('entries',  child_name) : child_GUID) element to the parent collection
                    modify_response = self.catalog.modifyMetadata({'_new' : (parent_GUID, 'add', 'entries', child_name, child_GUID)})
                    self.log('DEBUG', 'modifyMetadata response', modify_response)
                    # get the 'success' value
                    modify_success = modify_response['_new']
                    # if the new element was not set, we have a problem
                    if modify_success != 'set':
                        self.log('DEBUG', 'modifyMetadata failed, removing the new catalog entry', child_GUID)
                        # remove the newly created catalog-entry
                        self.catalog.remove({'_new' : child_GUID})
                        return 'failed to add child to parent', child_GUID
                    else:
                        return 'done', child_GUID
                else: # no parent given, skip the 'adding child to parent' part
                    return 'done', child_GUID
        except:
            self.log()
            return 'internal error', None

    def getFile(self, requests):
        """ Get files from the storage.
        
        getFile(requests)
        
        requests is a dictionary with requestID as key, and (Logical Name, protocol list) as value
        """
        # call the _traverse helper method the get the information about the requested Logical Names
        requests, traverse_response = self._traverse(requests)
        response = {}
        # for each requested LN
        for rID, (LN, protocols) in requests:
            turl = ''
            protocol = ''
            success = 'unknown'
            try:
                self.log('DEBUG', traverse_response[rID])
                # split the traverse response
                metadata, GUID, traversedLN, restLN, wasComplete, traversedList = traverse_response[rID]
                # wasComplete is true if the given LN was found, so it could have been fully traversed
                if not wasComplete:
                    success = 'not found'
                else:
                    # metadata contains all the metadata of the given entry
                    # ('catalog', 'type') is the type of the entry: file, collection, etc.
                    type = metadata[('catalog', 'type')]
                    if type != 'file':
                        success = 'is not a file'
                    else:
                        # if it is a file,  then we need all the locations where it is stored and alive
                        # this means all the metadata entries with in the 'locations' sections whose value is 'alive'
                        # the location itself serialized from the ID of the service and the ID of the replica within the service
                        # so the location needs to be deserialized into two ID with deserialize_ids()
                        # valid_locations will contain a list if (serviceID, referenceID, state)
                        valid_locations = [deserialize_ids(location) + [state] for (section, location), state in metadata.items() if section == 'locations' and state == 'alive']
                        # if this list is empty
                        if not valid_locations:
                            success = 'file has no valid replica'
                        else:
                            ok = False
                            while not ok and len(valid_locations) > 0:
                                # if there are more valid_locations, randomly select one
                                location = valid_locations.pop(random.choice(range(len(valid_locations))))
                                self.log('DEBUG', 'location chosen:', location)
                                # split it to serviceID, referenceID - serviceID currently is just a plain URL of the service
                                url, referenceID, _ = location
                                # create an ElementClient with this URL, then send a get request with the referenceID
                                #   we only support byteio protocol currently. 'getFile' is the requestID of this request
                                get_response = dict(ElementClient(url).get({'getFile' :
                                    [('referenceID', referenceID)] + [('protocol', proto) for proto in protocols]})['getFile'])
                                # get_response is a dictionary with keys such as 'TURL', 'protocol' or 'error'
                                if get_response.has_key('error'):
                                    # if there was an error
                                    self.log('DEBUG', 'ERROR', get_response['error'])
                                    success = 'error while getting TURL (%s)' % get_response['error']
                                else:
                                    # get the TURL and the choosen protocol, these will be set as reply for this requestID
                                    turl = get_response['TURL']
                                    protocol = get_response['protocol']
                                    success = 'done'
                                    ok = True
            except:
                success = 'internal error (%s)' % traceback.format_exc()
            # set the success, turl, protocol for this request
            response[rID] = (success, turl, protocol)
        return response
   
    def addReplica(self, requests, protocols):
        """ This method initiates the addition of a new replica to a file.
        
        addReplica(requests, protocols)
        
        requests is a dictionary with requestID-GUID pairs
        protocols is a list of supported protocols
        """
        # get the size and checksum information about all the requested GUIDs (these are in the 'states' section)
        #   the second argument of the get method specifies that we only need metadata from the 'states' section
        data = self.catalog.get(requests.values(), [('states',''),('locations','')])
        response = {}
        for rID, GUID in requests.items():
            # for each requested GUID
            states = data[GUID]
            self.log('DEBUG', 'addReplica', 'requestID', rID, 'GUID', GUID, 'states', states, 'protocols', protocols)
            # get the size and checksum information of the file
            size = states[('states','size')]
            checksumType = states[('states','checksumType')]
            checksum = states[('states','checksum')]
            # list of storage elements with a replica of this file (to avoid using one element twice)
            exceptedSEs = [deserialize_ids(location)[0] 
                           for (section, location), status in states.items() if section == 'locations']
            # initiate replica addition of this file with the given protocols 
            success, turl, protocol = self._add_replica(size, checksumType, checksum, GUID, protocols, exceptedSEs)
            # set the response of this request
            response[rID] = (success, turl, protocol)
        return response

    def find_alive_se(self, except_these=[]):
        """  Get the list of currently alive Storage Elements.
        
        find_alive_se()
        """
        # sestore_guid is the GUID of the catalog entry which the list of Storage Elements registered by the Catalog
        SEs = self.catalog.get([sestore_guid])[sestore_guid]
        # SEs contains entries such as {(serviceID, 'nextHeartbeat') : timestamp} which indicates
        #   when a specific Storage Element service should report next
        #   if this timestamp is not a positive number, that means the Storage Element have not reported in time, probably it is not alive
        self.log('DEBUG', 'Registered Storage Elements in Catalog', SEs)
        # get all the Storage Elements which has a positiv nextHeartbeat timestamp and which has not already been used
        alive_SEs = [s for (s, p), v in SEs.items() if p == 'nextHeartbeat' and int(v) > 0 and not s in except_these]
        self.log('DEBUG', 'Alive Storage Elements:', alive_SEs)
        if len(alive_SEs) == 0:
            return None
        try:
            # choose one randomly
            se = random.choice(alive_SEs)
            self.log('DEBUG', 'Storage Element chosen:', se)
            # the serviceID currently is a URL 
            # create an ElementClient with this URL
            return ElementClient(se)
        except:
            self.log()
            return None

    def _add_replica(self, size, checksumType, checksum, GUID, protocols, exceptedSEs=[]):
        """ Helper method to initiate addition of a replica to a file.
        
        _add_replica(size, checksumType, checksum, GUID, protocols)
        
        size is the size of the file
        checksumType indicates the type of the checksum
        checksum is the checksum itself
        GUID is the GUID of the file
        protocols is a list of protocols
        """
        turl = ''
        protocol = ''
        # prepare the 'put' request for the storage element
        put_request = [('size', size), ('checksumType', checksumType),
            ('checksum', checksum), ('GUID', GUID)] + \
            [('protocol', protocol) for protocol in protocols]
        # find an alive Storage Element
        element = self.find_alive_se(exceptedSEs)
        if not element:
            return 'no storage element found', turl, protocol
        # call the SE's put method with the prepared request
        put_response = dict(element.put({'putFile': put_request})['putFile'])
        if put_response.has_key('error'):
            self.log('DEBUG', 'ERROR', put_response['error'])
            # TODO: we should handle this, remove the new file or something
            return 'put error (%s)' % put_response['error'], turl, protocols
        else:
            # if the put request was successful then we have a transfer URL, a choosen protocol and the referenceID of the file
            turl = put_response['TURL']
            protocol = put_response['protocol']
            referenceID = put_response['referenceID']
            # currently the serviceID is the URL of the storage element service
            serviceID = element.url
            self.log('DEBUG', 'serviceID', serviceID, 'referenceID:', referenceID, 'turl', turl, 'protocol', protocol)
            # the serviceID and the referenceID is the location of the replica, serialized as one string
            # put the new location with the 'creating' state into the file entry ('putFile' is the requestID here)
            modify_response = self.catalog.modifyMetadata({'putFile' :
                    (GUID, 'set', 'locations', serialize_ids([serviceID, referenceID]), 'creating')})
            modify_success = modify_response['putFile']
            if modify_success != 'set':
                self.log('DEBUG', 'failed to add location to file', 'GUID', GUID, 'serviceID', serviceID, 'referenceID', referenceID)
                return 'failed to add new location to file', turl, protocol
                # TODO: error handling
            else:
                return 'done', turl, protocol
            
    def putFile(self, requests):
        """ Put a new file to the storage: initiate the process.
        
        putFile(requests)
        
        requests is a dictionary with requestID as key and (Logical Name, child metadata, protocols) as value
        """
        # get all the information about the requested Logical Names from the catalog
        requests, traverse_response = self._traverse(requests)
        response = {}
        for rID, (LN, child_metadata, protocols) in requests:
            # for each request
            turl = ''
            protocol = ''
            metadata_ok = False
            try:
                # get the size and checksum of the new file
                self.log('DEBUG', protocols)
                size = child_metadata[('states', 'size')]
                checksum = child_metadata[('states', 'checksum')]
                checksumType = child_metadata[('states', 'checksumType')]
                # need neededReplicas to see if we should call _add_replica
                neededReplicas = child_metadata[('states', 'neededReplicas')]
                metadata_ok = True
            except Exception, e:
                success = 'missing metadata ' + str(e)
            if metadata_ok:
                # if the metadata of the new file is OK
                try:
                    # split the Logical Name, rootguid will be the GUID of the root collection of this LN,
                    #   child_name is the name of the new file withing the parent collection
                    rootguid, _, child_name = splitLN(LN)
                    self.log('DEBUG', 'LN', LN, 'rootguid', rootguid, 'child_name', child_name, 'real rootguid', rootguid or global_root_guid)
                    # get the traverse response corresponding to this request
                    #   metadata is the metadata of the last element which could been traversed, e.g. the parent of the new file
                    #   GUID is the GUID of the same
                    #   traversedLN indicates which part of the requested LN could have been traversed
                    #   restLN is the non-traversed part of the Logical Name, e.g. the filename of a non-existent file whose parent does exist
                    #   wasComplete indicates if the traverse was complete or not, if it was complete means that this LN exists
                    #   traversedlist contains the GUID and metadata of each element along the path of the LN
                    metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[rID]
                    self.log('DEBUG', 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist)
                    if wasComplete: # this means the LN already exists, so we couldn't put a new file there
                        success = 'LN exists'
                    elif child_name == '': # this only can happen if the LN was a single GUID
                        # this means that the new file will have no parent
                        # set the type and GUID of the new file
                        child_metadata[('catalog','type')] = 'file'
                        child_metadata[('catalog','GUID')] = rootguid or global_root_guid
                        # create the new entry
                        success, GUID = self._new(child_metadata)
                    elif restLN != child_name or GUID == '':
                        # if the non-traversed part of the Logical Name is not actully the name of the new file
                        #   or we have no parent guid
                        success = 'parent does not exist'
                    else:
                        # if everything is OK, then we set the type of the new entry
                        child_metadata[('catalog','type')] = 'file'
                        # then create it
                        success, GUID = self._new(child_metadata, child_name, GUID)
                    if success == 'done':
                        # if the file was successfully created, it still has no replica, so we initiate creating one
                        # if neededReplicas is 0, we do nothing
                        if int(neededReplicas) > 0:
                            success, turl, protocol = self._add_replica(size, checksumType, checksum, GUID, protocols)
                except:
                    success = 'internal error (%s)' % traceback.format_exc()
            response[rID] = (success, turl, protocol)
        return response


    def makeCollection(self, requests):
        """ Create a new collection.
        
        makeCollection(requests)
        
        requests is dictionary with requestID as key and (Logical Name, metadata) as value
        """
        # do traverse all the requests
        requests, traverse_response = self._traverse(requests)
        response = {}
        for rID, (LN, child_metadata) in requests:
            # for each request first split the Logical Name
            rootguid, _, child_name = splitLN(LN)
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[rID]
            self.log('DEBUG', 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist)
            if wasComplete: # this means the LN exists
                success = 'LN exists'
            elif child_name == '': # this only can happen if the LN was a single GUID
                # this means the collection has no parent
                child_metadata[('catalog','type')] = 'collection'
                child_metadata[('catalog','GUID')] = rootguid or global_root_guid
                success, _ = self._new(child_metadata)
            elif restLN != child_name or GUID == '':
                success = 'parent does not exist'
            else:
                child_metadata[('catalog','type')] = 'collection'
                # if everything is OK, create the new collection
                #   here GUID is of the parent collection
                success, _ = self._new(child_metadata, child_name, GUID)
            response[rID] = success
        return response

    def list(self, requests, neededMetadata = []):
        """ List the contents of a collection.
        
        list(requests, neededMetadata = [])
        
        requests is a dictionary with requestID as key and Logical Name as value
        neededMetadata is a list of (section, property) where property could be empty which means all properties of that section
            if neededMetadata is empty it means we need everything
        """
        # do traverse the requested Logical Names
        traverse_response = self.catalog.traverseLN(requests)
        response = {}
        for requestID, LN in requests.items():
            # for each LN
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[requestID]
            if wasComplete:
                # this means the LN exists, get its type
                type = metadata[('catalog', 'type')]
                if type == 'file': # files have no contents, we do not list them
                    status = 'is a file'
                    entries = {}
                else: # if it is not a file, it must be a collection (currently there is no other type)
                    status = 'found'
                    # get all the properties and values from the 'entries' metadata section of the collection
                    #   these are the names and GUIDs: the contents of the collection
                    GUIDs = dict([(name, GUID)
                        for (section, name), GUID in metadata.items() if section == 'entries'])
                    # get the needed metadata of all the entries
                    metadata = self.catalog.get(GUIDs.values(), neededMetadata)
                    # create a dictionary with the name of the entry as key and (GUID, metadata) as value
                    entries = dict([(name, (GUID, metadata[GUID])) for name, GUID in GUIDs.items()])
            else:
                entries = {}
                status = 'not found'
            response[requestID] = (entries, status)
        return response

    def move(self, requests):
        """ Move a file or collection within the global namespace.
        
        move(requests)
        
        requests is a dictionary with requestID as key and
            (sourceLN, targetLN, preserverOriginal) as value
        if preserverOriginal is true this method creates a hard link instead of moving
        """
        traverse_request = {}
        # create a traverse request, each move request needs two traversing: source and target
        for requestID, (sourceLN, targetLN, _) in requests.items():
            # from one requestID we create two: one for the source and one for the target
            traverse_request[requestID + 'source'] = sourceLN
            traverse_request[requestID + 'target'] = targetLN
        traverse_response = self.catalog.traverseLN(traverse_request)
        self.log('DEBUG', '\/\/', traverse_response)
        response = {}
        for requestID, (sourceLN, targetLN, preserveOriginal) in requests.items():
            # for each request
            self.log('DEBUG', requestID, sourceLN, targetLN, preserveOriginal)
            # get the old and the new name of the entry, these are the last elements of the Logical Names
            _, _, old_child_name = splitLN(sourceLN)
            _, _, new_child_name = splitLN(targetLN)
            # get the GUID of the source LN from the traverse response
            _, sourceGUID, _, _, sourceWasComplete, sourceTraversedList \
                = traverse_response[requestID + 'source']
            # get the GUID form the target's traverse response, this should be the parent of the target LN
            _, targetGUID, _, targetRestLN, targetWasComplete, _ \
                = traverse_response[requestID + 'target']
            # if the source traverse was not complete: the source LN does not exist
            if not sourceWasComplete:
                success = 'nosuchLN'
            # if the target traverse was complete: the target LN already exists
            #   but if the new_child_name was empty, then the target is considered to be a collection, so it is OK
            elif targetWasComplete and new_child_name != '':
                success = 'targetexists'
            # if the target traverse was not complete, and the non-traversed part is not just the new name, we have a problem
            elif not targetWasComplete and targetRestLN != new_child_name:
                success = 'invalidtarget'
            else:
                # if the new child name is empty that means that the target LN has a trailing slash
                #   so we just put the old name after it
                if new_child_name == '':
                    new_child_name = old_child_name
                self.log('DEBUG', 'adding', sourceGUID, 'to parent', targetGUID)
                # adding the entry to the new parent
                mm_resp = self.catalog.modifyMetadata(
                    {'move' : (targetGUID, 'add', 'entries', new_child_name, sourceGUID),
                        'parent' : (sourceGUID, 'set', 'parents', '%s/%s' % (targetGUID, new_child_name), 'parent')})
                mm_succ = mm_resp['move']
                if mm_succ != 'set':
                    success = 'failed adding child to parent'
                else:
                    if preserveOriginal == true:
                        success = 'moved'
                    else:
                        # then we need to remove the source LN
                        # get the parent of the source: the source traverse has a list of the GUIDs of all the element along the path
                        source_parent_guid = sourceTraversedList[-2][1]
                        self.log('DEBUG', 'removing', sourceGUID, 'from parent', source_parent_guid)
                        # delete the entry from the source parent
                        mm_resp = self.catalog.modifyMetadata(
                            {'move' : (source_parent_guid, 'unset', 'entries', old_child_name, ''),
                                'parent' : (sourceGUID, 'unset', 'parents', '%s/%s' % (source_parent_guid, old_child_name), '')})
                        mm_succ = mm_resp['move']
                        if mm_succ != 'unset':
                            success = 'failed removing child from parent'
                            # TODO: need some handling; remove the new entry or something
                        else:
                            success = 'moved'
            response[requestID] = success
        return response

    def modify(self, requests):
        requests, traverse_response = self._traverse(requests)
        catalog_requests = {}
        not_found = []
        for changeID, (LN, changeType, section, property, value) in requests:
            _, GUID, _, _, wasComplete, _ = traverse_response[changeID]
            if wasComplete:
                catalog_requests[changeID] = (GUID, changeType, section, property, value)
            else:
                not_found.append(changeID)
        response = self.catalog.modifyMetadata(catalog_requests)
        for changeID in not_found:
            response[changeID] = 'no such LN'
        return response

from storage.service import Service

class ManagerService(Service):

    def __init__(self, cfg):
        # names of provided methods
        request_names = ['stat', 'makeCollection', 'list', 'move', 'putFile', 'getFile', 'addReplica', 'delFile', 'modify']
        # call the Service's constructor, 'Manager' is the human-readable name of the service
        # request_names is the list of the names of the provided methods
        # manager_uri is the URI of the Manager service namespace, and 'man' is the prefix we want to use for this namespace
        Service.__init__(self, 'Manager', request_names, 'man', manager_uri, cfg)
        # get the URL of the Catalog from the config file
        catalog_url = str(cfg.Get('CatalogURL'))
        # create a CatalogClient from the URL
        catalog = CatalogClient(catalog_url)
        self.manager = Manager(catalog)
        self.manager.log = self.log

    def stat(self, inpayload):
        # incoming SOAP message example:
        #
        #   <man:stat>
        #       <man:statRequestList>
        #           <man:statRequestElement>
        #               <man:requestID>0</man:requestID>
        #               <man:LN>/</man:LN>
        #           </man:statRequestElement>
        #       </man:statRequestList>
        #   </man:stat>
        #
        # outgoing SOAP message example:
        #
        #   <man:statResponse>
        #       <man:statResponseList>
        #           <man:statResponseElement>
        #              <man:requestID>0</man:requestID>
        #               <man:metadataList>
        #                   <man:metadata>
        #                       <man:section>states</man:section>
        #                       <man:property>closed</man:property>
        #                       <man:value>0</man:value>
        #                   </man:metadata>
        #                   <man:metadata>
        #                       <man:section>entries</man:section>
        #                       <man:property>testfile</man:property>
        #                       <man:value>cf05727b-73f3-4318-8454-16eaf10f302c</man:value>
        #                   </man:metadata>
        #                   <man:metadata>
        #                       <man:section>catalog</man:section>
        #                       <man:property>type</man:property>
        #                       <man:value>collection</man:value>
        #                   </man:metadata>
        #               </man:metadataList>
        #           </man:statResponseElement>
        #       </man:statResponseList>
        #   </man:statResponse>

        # get all the requests
        request_nodes = get_child_nodes(inpayload.Child().Child())
        # get the requestID and LN of each request and create a dictionary where the requestID is the key and the LN is the value
        requests = dict([
            (str(request_node.Get('requestID')), str(request_node.Get('LN')))
                for request_node in request_nodes
        ])
        # call the Manager class
        response = self.manager.stat(requests)
        # create the metadata XML structure of each request
        for requestID, metadata in response.items():
            response[requestID] = create_metadata(metadata, 'man')
        # create the response message with the requestID and the metadata for each request
        return create_response('man:stat',
            ['man:requestID', 'man:metadataList'], response, self.newSOAPPayload(), single = True)

    def delFile(self, inpayload):
        # incoming SOAP message example:
        #
        #   <man:delFile>
        #       <man:delFileRequestList>
        #           <man:delFileRequestElement>
        #               <man:requestID>0</man:requestID>
        #               <man:LN>/</man:LN>
        #           </man:delFileRequestElement>
        #       </man:delFileRequestList>
        #   </man:delFile>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <man:delFileResponse>
        #               <man:delFileResponseList>
        #                   <man:delFileResponseElement>
        #                       <man:requestID>0</man:requestID>
        #                       <man:success>deleted</man:success>
        #                   </man:delFileResponseElement>
        #               </man:delFileResponseList>
        #           </man:delFileResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        request_nodes = get_child_nodes(inpayload.Child().Child())
        # get the requestID and LN of each request and create a dictionary where the requestID is the key and the LN is the value
        requests = dict([
            (str(request_node.Get('requestID')), str(request_node.Get('LN')))
                for request_node in request_nodes
        ])
        response = self.manager.delFile(requests)
        return create_response('man:delFile',
            ['man:requestID', 'man:success'], response, self.newSOAPPayload(), single=True)


    def getFile(self, inpayload):
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <man:getFile>
        #           <man:getFileRequestList>
        #               <man:getFileRequestElement>
        #                   <man:requestID>0</man:requestID>
        #                   <man:LN>/testfile</man:LN>
        #                   <man:protocol>byteio</man:protocol>
        #               </man:getFileRequestElement>
        #           </man:getFileRequestList>
        #       </man:getFile>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <man:getFileResponse>
        #               <man:getFileResponseList>
        #                   <man:getFileResponseElement>
        #                       <man:requestID>0</man:requestID>
        #                       <man:success>done</man:success>
        #                       <man:TURL>http://localhost:60000/byteio/12d86ba3-99d5-408e-91d1-35f7c47774e4</man:TURL>
        #                       <man:protocol>byteio</man:protocol>
        #                   </man:getFileResponseElement>
        #               </man:getFileResponseList>
        #           </man:getFileResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                ( # get the LN and all the protocols for each request and put them in a list
                    str(request_node.Get('LN')),
                    [str(node) for node in request_node.XPathLookup('//man:protocol', self.ns)]
                )
            ) for request_node in request_nodes
        ])
        response = self.manager.getFile(requests)
        return create_response('man:getFile',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.newSOAPPayload())
    
    def addReplica(self, inpayload):
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <man:addReplica>
        #           <man:addReplicaRequestList>
        #               <man:putReplicaRequestElement>
        #                   <man:requestID>0</man:requestID>
        #                   <man:GUID>cf05727b-73f3-4318-8454-16eaf10f302c</man:GUID>
        #               </man:putReplicaRequestElement>
        #           </man:addReplicaRequestList>
        #           <man:protocol>byteio</man:protocol>
        #       </man:addReplica>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <man:addReplicaResponse>
        #               <man:addReplicaResponseList>
        #                   <man:addReplicaResponseElement>
        #                       <man:requestID>0</man:requestID>
        #                       <man:success>done</man:success>
        #                       <man:TURL>http://localhost:60000/byteio/f568be18-26ae-4925-ae0c-68fe023ef1a5</man:TURL>
        #                       <man:protocol>byteio</man:protocol>
        #                   </man:addReplicaResponseElement>
        #               </man:addReplicaResponseList>
        #           </man:addReplicaResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>
        
        # get the list of supported protocols
        protocols = [str(node) for node in inpayload.XPathLookup('//man:protocol', self.ns)]
        # get the GUID of each request
        request_nodes = get_child_nodes(inpayload.Child().Get('addReplicaRequestList'))
        requests = dict([(str(request_node.Get('requestID')), str(request_node.Get('GUID')))
                for request_node in request_nodes])
        response = self.manager.addReplica(requests, protocols)
        return create_response('man:addReplica',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.newSOAPPayload())
    
    def putFile(self, inpayload):
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <man:putFile>
        #           <man:putFileRequestList>
        #               <man:putFileRequestElement>
        #                   <man:requestID>0</man:requestID>
        #                   <man:LN>/testfile2</man:LN>
        #                   <man:metadataList>
        #                       <man:metadata>
        #                           <man:section>states</man:section>
        #                           <man:property>neededReplicas</man:property>
        #                           <man:value>2</man:value>
        #                       </man:metadata>
        #                       <man:metadata>
        #                           <man:section>states</man:section>
        #                           <man:property>size</man:property>
        #                           <man:value>11</man:value>
        #                       </man:metadata>
        #                   </man:metadataList>
        #                   <man:protocol>byteio</man:protocol>
        #               </man:putFileRequestElement>
        #           </man:putFileRequestList>
        #       </man:putFile>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <man:putFileResponse>
        #               <man:putFileResponseList>
        #                   <man:putFileResponseElement>
        #                       <man:requestID>0</man:requestID>
        #                       <man:success>done</man:success>
        #                       <man:TURL>http://localhost:60000/byteio/b8f2987b-a718-47b3-82bb-e838470b7e00</man:TURL>
        #                       <man:protocol>byteio</man:protocol>
        #                   </man:putFileResponseElement>
        #               </man:putFileResponseList>
        #           </man:putFileResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                (
                    str(request_node.Get('LN')),
                    parse_metadata(request_node.Get('metadataList')),
                    [str(node) for node in request_node.XPathLookup('//man:protocol', self.ns)]
                )
            ) for request_node in request_nodes
        ])
        response = self.manager.putFile(requests)
        return create_response('man:putFile',
            ['man:requestID', 'man:success', 'man:TURL', 'man:protocol'], response, self.newSOAPPayload())

    def makeCollection(self, inpayload):
        # incoming SOAP message example:
        # 
        #   <soap-env:Body>
        #       <man:makeCollection>
        #           <man:makeCollectionRequestList>
        #               <man:makeCollectionRequestElement>
        #                   <man:requestID>0</man:requestID>
        #                   <man:LN>/testdir</man:LN>
        #                   <man:metadataList>
        #                       <man:metadata>
        #                           <man:section>states</man:section>
        #                           <man:property>closed</man:property>
        #                           <man:value>0</man:value>
        #                       </man:metadata>
        #                   </man:metadataList>
        #               </man:makeCollectionRequestElement>
        #           </man:makeCollectionRequestList>
        #       </man:makeCollection>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <man:makeCollectionResponse>
        #               <man:makeCollectionResponseList>
        #                   <man:makeCollectionResponseElement>
        #                       <man:requestID>0</man:requestID>
        #                       <man:success>done</man:success>
        #                   </man:makeCollectionResponseElement>
        #               </man:makeCollectionResponseList>
        #           </man:makeCollectionResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

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
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <man:list>
        #           <man:listRequestList>
        #               <man:listRequestElement>
        #                   <man:requestID>0</man:requestID>
        #                   <man:LN>/</man:LN>
        #               </man:listRequestElement>
        #           </man:listRequestList>
        #           <man:neededMetadataList>
        #               <man:neededMetadataElement>
        #                   <man:section>catalog</man:section>
        #                   <man:property></man:property>
        #               </man:neededMetadataElement>
        #           </man:neededMetadataList>
        #       </man:list>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <man:listResponse>
        #               <man:listResponseList>
        #                   <man:listResponseElement>
        #                       <man:requestID>0</man:requestID>
        #                       <man:entries>
        #                           <man:entry>
        #                               <man:name>testfile</man:name>
        #                               <man:GUID>cf05727b-73f3-4318-8454-16eaf10f302c</man:GUID>
        #                               <man:metadataList>
        #                                   <man:metadata>
        #                                       <man:section>catalog</man:section>
        #                                       <man:property>type</man:property>
        #                                       <man:value>file</man:value>
        #                                   </man:metadata>
        #                               </man:metadataList>
        #                           </man:entry>
        #                           <man:entry>
        #                               <man:name>testdir</man:name>
        #                               <man:GUID>4cabc8cb-599d-488c-a253-165f71d4e180</man:GUID>
        #                               <man:metadataList>
        #                                   <man:metadata>
        #                                       <man:section>catalog</man:section>
        #                                       <man:property>type</man:property>
        #                                       <man:value>collection</man:value>
        #                                   </man:metadata>
        #                               </man:metadataList>
        #                           </man:entry>
        #                       </man:entries>
        #                       <man:status>found</man:status>
        #                   </man:listResponseElement>
        #               </man:listResponseList>
        #           </man:listResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

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
        # incoming SOAP message example:
        #   <soap-env:Body>
        #       <man:move>
        #           <man:moveRequestList>
        #               <man:moveRequestElement>
        #                   <man:requestID>0</man:requestID>
        #                   <man:sourceLN>/testfile2</man:sourceLN>
        #                   <man:targetLN>/testdir/</man:targetLN>
        #                   <man:preserveOriginal>0</man:preserveOriginal>
        #               </man:moveRequestElement>
        #           </man:moveRequestList>
        #       </man:move>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <man:moveResponse>
        #               <man:moveResponseList>
        #                   <man:moveResponseElement>
        #                       <man:requestID>0</man:requestID>
        #                       <man:status>moved</man:status>
        #                   </man:moveResponseElement>
        #               </man:moveResponseList>
        #           </man:moveResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        requests = parse_node(inpayload.Child().Child(),
            ['requestID', 'sourceLN', 'targetLN', 'preserveOriginal'])
        response = self.manager.move(requests)
        return create_response('man:move',
            ['man:requestID', 'man:status'], response, self.newSOAPPayload(), single = True)

    def modify(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['man:changeID',
            'man:LN', 'man:changeType', 'man:section', 'man:property', 'man:value'])
        response = self.manager.modify(requests)
        return create_response('man:modify', ['man:changeID', 'man:success'],
            response, self.newSOAPPayload(), single = True)
