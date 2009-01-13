import urlparse
import httplib
import arc
import random
import time
import traceback
from storage.xmltree import XMLTree
from storage.client import LibrarianClient, ShepherdClient
from storage.common import parse_metadata, librarian_uri, bartender_uri, create_response, create_metadata, true, \
                            splitLN, remove_trailing_slash, get_child_nodes, parse_node, node_to_data, global_root_guid, \
                            serialize_ids, deserialize_ids, sestore_guid, parse_ssl_config, make_decision_metadata, create_owner_policy
import traceback

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.Bartender'))

class Bartender:

    def __init__(self, librarian, ssl_config):
        """ Constructor of the Bartender business-logic class.
        
        Bartender(librarian)
        
        librarian is LibrarianClient object which can be used to access a Librarian service
        """
        self.librarian = librarian
        self.ssl_config = ssl_config

    def stat(self, auth, requests):
        """ Returns stat information about entries.
        
        stat(auth, requests)
        
        auth is an AuthRequest object containing information about the user's identity
        requests is a dictionary with requestIDs as keys, and Logical Names as values.
    
        Returns a dictionary with requestIDs as keys, and metadata as values.
        The 'metadata' is a dictionary with (section, property) pairs as keys.
        """
        response = {}
        # get the information from the librarian
        traverse_response = self.librarian.traverseLN(requests)
        # we are only interested in the metadata and if the traversing was complete or not
        for requestID, (metadata, _, _, _, wasComplete, _) in traverse_response.items():
            if wasComplete: # if it was complete, then we found the entry and got the metadata
                try:
                    # decision = make_decision_metadata(metadata, auth.get_request('read'))
                    decision = arc.DECISION_PERMIT
                    if decision != arc.DECISION_PERMIT:
                        metadata = {('error','permission denied') : 'you are not allowed to read'}
                except:
                    log.msg()
                    metadata = {('error', 'error checking permission') : traceback.format_exc()}
                response[requestID] = metadata
            else: # if it was not complete, then we didn't found the entry, so metadata will be empty
                response[requestID] = {}
        return response
    
    def delFile(self, auth, requests):
        """ Delete a file from the storage: initiate the process.
        
        delFile(requests)
        
        requests is a dictionary with requestID as key and (Logical Name, child metadata, protocols) as value
        """
        auth_request = auth.get_request('delete')
        import time
        response = {}
        # get the information from the librarian
        #requests, traverse_response = self._traverse(requests)
        traverse_response = self.librarian.traverseLN(requests)
        cat_rem_requests = {}
        cat_mod_requests = {}
        check_again = []
        for requestID, (metadata, GUID, LN, _, wasComplete, traversedList) in traverse_response.items():
            # decision = make_decision_metadata(metadata, auth_request)
            decision = arc.DECISION_PERMIT
            if decision != arc.DECISION_PERMIT:
                response[requestID] = 'denied'
            elif wasComplete and metadata[('entry', 'type')]=='file': # if it was complete, then we found the entry and got the metadata
                parentno = len([property for (section, property), value in metadata.items() if section == 'parents'])
                if parentno < 2:
                    # remove the file itself,  if this file has only one parent (hardlink), or has no parent at all
                    cat_rem_requests[requestID] = GUID
                # if this entry has a parent:
                if len(traversedList)>1:
                    # notify the parent collection
                    # TODO: get the metadata of the parent collection and check if the user has permission to removeEntry from it
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
        success = self.librarian.remove(cat_rem_requests)
        modify_success = self.librarian.modifyMetadata(cat_mod_requests)
        # check the files with more parents (hardlinks) again
        if check_again:
            log.msg(arc.DEBUG, '\n\n!!!!check_again', check_again)
            checked_again = self.librarian.get(check_again)
            do_delete = {}
            for GUID, metadata in checked_again.items():
                # if a file has no parents now, remove it
                parentno = len([property for (section, property), value in metadata.items() if section == 'parents'])
                if parentno == 0:
                    do_delete[GUID] = GUID
            log.msg(arc.DEBUG, '\n\n!!!!do_delete', do_delete)
            self.librarian.remove(do_delete)
        return response
            

    def _traverse(self, requests):
        """ Helper method which connects the librarian, and traverses the LNs of the requests.
        
        _traverse(requests)
        
        Removes the trailing slash from  all the LNs in the request.
        Returns the requests and the traverse response.
        """
        # in each request the requestID is the key and the value is a list
        # the first item of the list is the Logical Name, we want to remove the trailing slash, and
        # leave the other items intact
        requests = [(rID, [remove_trailing_slash(data[0])] + list(data[1:])) for rID, data in requests.items()]
        log.msg(arc.DEBUG, '//// _traverse request trailing slash removed:', dict(requests))
        # then we do the traversing. a traverse request contains a requestID and the Logical Name
        # so we just need the key (which is the request ID) and the first item of the value (which is the LN)
        traverse_request = dict([(rID, data[0]) for rID, data in requests])
        # call the librarian service
        traverse_response = self.librarian.traverseLN(traverse_request)
        # return the requests as list (without the trailing slashes) and the traverse response from the librarian
        return requests, traverse_response

    def _new(self, auth, child_metadata, child_name = None, parent_GUID = None, parent_metadata = {}):
        """ Helper method which create a new entry in the librarian.
        
        _new(child_metadata, child_name = None, parent_GUID = None)
        
        child_metadata is a dictionary with {(section, property) : values} containing the metadata of the new entry
        child_name is the name of the new entry 
        parent_GUID is the GUID of the parent of the new entry
        
        This method creates a new librarian-entry with the given metadata.
        If child_name and parent_GUID are both given, then this method adds a new entry to the parent collection.
        """
        try:
            # set creation time stamp
            child_metadata[('timestamps', 'created')] = str(time.time())
            # set owner's permissions
            if child_name and parent_GUID:
                child_metadata[('parents', '%s/%s' % (parent_GUID, child_name))] = 'parent'
            # call the new method of the librarian with the child's metadata (requestID is '_new')
            new_response = self.librarian.new({'_new' : child_metadata})
            # we can access the response with the requestID, so we get the GUID of the newly created entry
            (child_GUID, new_success) = new_response['_new']
            # if the new method was not successful
            if new_success != 'success':
                return 'failed to create new librarian entry', child_GUID
            else:
                # if it was successful and we have a parent collection
                if child_name and parent_GUID:
                    # decision = make_decision_metadata(parent_metadata, auth.get_request('addEntry))
                    decision = arc.DECISION_PERMIT
                    if decision == arc.DECISION_PERMIT:
                        # we need to add the newly created librarian-entry to the parent collection
                        log.msg(arc.DEBUG, 'adding', child_GUID, 'to parent', parent_GUID)
                        # this modifyMetadata request adds a new (('entries',  child_name) : child_GUID) element to the parent collection
                        modify_response = self.librarian.modifyMetadata({'_new' : (parent_GUID, 'add', 'entries', child_name, child_GUID)})
                        log.msg(arc.DEBUG, 'modifyMetadata response', modify_response)
                        # get the 'success' value
                        modify_success = modify_response['_new']
                    else:
                        modify_success = 'denied'
                    # if the new element was not set, we have a problem
                    if modify_success != 'set':
                        log.msg(arc.DEBUG, 'modifyMetadata failed, removing the new librarian entry', child_GUID)
                        # remove the newly created librarian-entry
                        self.librarian.remove({'_new' : child_GUID})
                        return 'failed to add child to parent', child_GUID
                    else:
                        return 'done', child_GUID
                else: # no parent given, skip the 'adding child to parent' part
                    return 'done', child_GUID
        except:
            log.msg()
            return 'internal error', None

    def getFile(self, auth, requests):
        """ Get files from the storage.
        
        getFile(requests)
        
        requests is a dictionary with requestID as key, and (Logical Name, protocol list) as value
        """
        auth_request = auth.get_request('read')
        # call the _traverse helper method the get the information about the requested Logical Names
        requests, traverse_response = self._traverse(requests)
        response = {}
        # for each requested LN
        for rID, (LN, protocols) in requests:
            turl = ''
            protocol = ''
            success = 'unknown'
            try:
                log.msg(arc.DEBUG, traverse_response[rID])
                # split the traverse response
                metadata, GUID, traversedLN, restLN, wasComplete, traversedList = traverse_response[rID]
                # wasComplete is true if the given LN was found, so it could have been fully traversed
                if not wasComplete:
                    success = 'not found'
                else:
                    # metadata contains all the metadata of the given entry
                    # ('entry', 'type') is the type of the entry: file, collection, etc.
                    #decision = make_decision_metadata(metadata, auth_request)
                    decision = arc.DECISION_PERMIT
                    if decision != arc.DECISION_PERMIT:
                        success = 'denied'
                    else:
                        type = metadata[('entry', 'type')]
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
                                    log.msg(arc.DEBUG, 'location chosen:', location)
                                    # split it to serviceID, referenceID - serviceID currently is just a plain URL of the service
                                    url, referenceID, _ = location
                                    # create an ShepherdClient with this URL, then send a get request with the referenceID
                                    #   we only support byteio protocol currently. 'getFile' is the requestID of this request
                                    get_response = dict(ShepherdClient(url, ssl_config = self.ssl_config).get({'getFile' :
                                        [('referenceID', referenceID)] + [('protocol', proto) for proto in protocols]})['getFile'])
                                    # get_response is a dictionary with keys such as 'TURL', 'protocol' or 'error'
                                    if get_response.has_key('error'):
                                        # if there was an error
                                        log.msg(arc.DEBUG, 'ERROR', get_response['error'])
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
   
    def addReplica(self, auth, requests, protocols):
        """ This method initiates the addition of a new replica to a file.
        
        addReplica(requests, protocols)
        
        requests is a dictionary with requestID-GUID pairs
        protocols is a list of supported protocols
        """
        auth_request = auth.get_request('addEntry')
        # get the size and checksum information about all the requested GUIDs (these are in the 'states' section)
        #   the second argument of the get method specifies that we only need metadata from the 'states' section
        data = self.librarian.get(requests.values(), [('states',''),('locations',''),('policy','')])
        response = {}
        for rID, GUID in requests.items():
            # for each requested GUID
            metadata = data[GUID]
            #decision = make_decision_metadata(metadata, auth_request)
            decision = arc.DECISION_PERMIT
            if decision != arc.DECISION_PERMIT:
                response[rID] = ('denied', None, None)
            else:
                log.msg(arc.DEBUG, 'addReplica', 'requestID', rID, 'GUID', GUID, 'metadata', metadata, 'protocols', protocols)
                # get the size and checksum information of the file
                size = metadata[('states','size')]
                checksumType = metadata[('states','checksumType')]
                checksum = metadata[('states','checksum')]
                # list of shepherds with a replica of this file (to avoid using one shepherd twice)
                exceptedSEs = [deserialize_ids(location)[0] 
                               for (section, location), status in metadata.items() if section == 'locations']
                # initiate replica addition of this file with the given protocols 
                success, turl, protocol = self._add_replica(size, checksumType, checksum, GUID, protocols, exceptedSEs)
                # set the response of this request
                response[rID] = (success, turl, protocol)
        return response

    def _find_alive_se(self, except_these=[]):
        """  Get the list of currently alive Shepherds.
        
        _find_alive_se()
        """
        # sestore_guid is the GUID of the librarian entry which the list of Shepherds registered by the Librarian
        SEs = self.librarian.get([sestore_guid])[sestore_guid]
        # SEs contains entries such as {(serviceID, 'nextHeartbeat') : timestamp} which indicates
        #   when a specific Shepherd service should report next
        #   if this timestamp is not a positive number, that means the Shepherd have not reported in time, probably it is not alive
        log.msg(arc.DEBUG, 'Registered Shepherds in Librarian', SEs)
        # get all the Shepherds which has a positiv nextHeartbeat timestamp and which has not already been used
        alive_SEs = [s for (s, p), v in SEs.items() if p == 'nextHeartbeat' and int(v) > 0 and not s in except_these]
        log.msg(arc.DEBUG, 'Alive Shepherds:', alive_SEs)
        if len(alive_SEs) == 0:
            return None
        try:
            # choose one randomly
            se = random.choice(alive_SEs)
            log.msg(arc.DEBUG, 'Shepherd chosen:', se)
            # the serviceID currently is a URL 
            # create an ShepherdClient with this URL
            return ShepherdClient(se, ssl_config = self.ssl_config)
        except:
            log.msg()
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
        # prepare the 'put' request for the shepherd
        put_request = [('size', size), ('checksumType', checksumType),
            ('checksum', checksum), ('GUID', GUID)] + \
            [('protocol', protocol) for protocol in protocols]
        # find an alive Shepherd
        shepherd = self._find_alive_se(exceptedSEs)
        if not shepherd:
            return 'no shepherd found', turl, protocol
        # call the SE's put method with the prepared request
        put_response = dict(shepherd.put({'putFile': put_request})['putFile'])
        if put_response.has_key('error'):
            log.msg(arc.DEBUG, 'ERROR', put_response['error'])
            # TODO: we should handle this, remove the new file or something
            return 'put error (%s)' % put_response['error'], turl, protocols
        else:
            # if the put request was successful then we have a transfer URL, a choosen protocol and the referenceID of the file
            # TODO: check if this is working this way
            turl = put_response['TURL']
            protocol = put_response['protocol']
            return 'done', turl, protocol
            #referenceID = put_response['referenceID']
            ## currently the serviceID is the URL of the shepherd service
            #serviceID = shepherd.url
            #log.msg(arc.DEBUG, 'serviceID', serviceID, 'referenceID:', referenceID, 'turl', turl, 'protocol', protocol)
            ## the serviceID and the referenceID is the location of the replica, serialized as one string
            ## put the new location with the 'creating' state into the file entry ('putFile' is the requestID here)
            #modify_response = self.librarian.modifyMetadata({'putFile' :
            #        (GUID, 'set', 'locations', serialize_ids([serviceID, referenceID]), 'creating')})
            #modify_success = modify_response['putFile']
            #if modify_success != 'set':
            #    log.msg(arc.DEBUG, 'failed to add location to file', 'GUID', GUID, 'serviceID', serviceID, 'referenceID', referenceID)
            #    return 'failed to add new location to file', turl, protocol
            #    # TODO: error handling
            #else:
            #    return 'done', turl, protocol
            
    def putFile(self, auth, requests):
        """ Put a new file to the storage: initiate the process.
        
        putFile(requests)
        
        requests is a dictionary with requestID as key and (Logical Name, child metadata, protocols) as value
        """
        # get all the information about the requested Logical Names from the librarian
        requests, traverse_response = self._traverse(requests)
        response = {}
        for rID, (LN, child_metadata, protocols) in requests:
            # for each request
            turl = ''
            protocol = ''
            metadata_ok = False
            try:
                # get the size and checksum of the new file
                log.msg(arc.DEBUG, protocols)
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
                    log.msg(arc.DEBUG, 'LN', LN, 'rootguid', rootguid, 'child_name', child_name, 'real rootguid', rootguid or global_root_guid)
                    # get the traverse response corresponding to this request
                    #   metadata is the metadata of the last element which could been traversed, e.g. the parent of the new file
                    #   GUID is the GUID of the same
                    #   traversedLN indicates which part of the requested LN could have been traversed
                    #   restLN is the non-traversed part of the Logical Name, e.g. the filename of a non-existent file whose parent does exist
                    #   wasComplete indicates if the traverse was complete or not, if it was complete means that this LN exists
                    #   traversedlist contains the GUID and metadata of each element along the path of the LN
                    metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[rID]
                    log.msg(arc.DEBUG, 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist)
                    if wasComplete: # this means the LN already exists, so we couldn't put a new file there
                        success = 'LN exists'
                    elif child_name == '': # this only can happen if the LN was a single GUID
                        # this means that the new file will have no parent
                        # set the type and GUID of the new file
                        child_metadata[('entry','type')] = 'file'
                        child_metadata[('entry','GUID')] = rootguid or global_root_guid
                        # create the new entry
                        success, GUID = self._new(auth, child_metadata)
                    elif restLN != child_name or GUID == '':
                        # if the non-traversed part of the Logical Name is not actully the name of the new file
                        #   or we have no parent guid
                        success = 'parent does not exist'
                    else:
                        # if everything is OK, then we set the type of the new entry
                        child_metadata[('entry','type')] = 'file'
                        # then create it
                        success, GUID = self._new(auth, child_metadata, child_name, GUID)
                    if success == 'done':
                        # if the file was successfully created, it still has no replica, so we initiate creating one
                        # if neededReplicas is 0, we do nothing
                        if int(neededReplicas) > 0: # this will call shepherd.put()
                            success, turl, protocol = self._add_replica(size, checksumType, checksum, GUID, protocols)
                except:
                    success = 'internal error (%s)' % traceback.format_exc()
            response[rID] = (success, turl, protocol)
        return response

    def unmakeCollection(self, auth, requests):
        """docstring for unmakeCollection"""
        auth_request = auth.get_request('delete')
        requests, traverse_response = self._traverse(requests)
        response = {}
        for rID, [LN] in requests:
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[rID]
            log.msg(arc.DEBUG, 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist)
            if not wasComplete:
                success = 'no such LN'
            else:
                #decision = make_decision_metadata(metadata, auth_request)
                decision = arc.DECISION_PERMIT
                if decision != arc.DECISION_PERMIT:
                    success = 'denied'
                else:
                    number_of_entries = len([section for (section, _), _ in metadata.items() if section == 'entries'])
                    if number_of_entries > 0:
                        success = 'collection is not empty'
                    else:
                        try:
                            parentLN, parentGUID = traversedlist[-2]
                            # TODO: get the metadata of the parent, and check if the user has permission to removeEntry from it
                            mod_requests = {'unmake' : (parentGUID, 'unset', 'entries', traversedlist[-1][0], '')}
                            mod_response = self.librarian.modifyMetadata(mod_requests)
                            success = mod_response['unmake']
                        except IndexError:
                            # it has no parent
                            success = 'unset'
                        if success == 'unset':
                            # TODO: handle hardlinks to collections
                            success = self.librarian.remove({'unmake' : GUID})['unmake']
            response[rID] = success
        return response

    def makeCollection(self, auth, requests):
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
            log.msg(arc.DEBUG, 'metadata', metadata, 'GUID', GUID, 'traversedLN', traversedLN, 'restLN', restLN, 'wasComplete',wasComplete, 'traversedlist', traversedlist)
            owner_identity = auth.get_identity()
            if owner_identity:
                owner_policy = create_owner_policy(owner_identity).get_policy('StorageAuth')
                print owner_policy
                for identity, actions in owner_policy:
                    child_metadata[('policy', identity)] = actions
            child_metadata[('entry','type')] = 'collection'
            if wasComplete: # this means the LN exists
                success = 'LN exists'
            elif child_name == '': # this only can happen if the LN was a single GUID
                # this means the collection has no parent
                child_metadata[('entry','GUID')] = rootguid or global_root_guid
                print child_metadata
                success, _ = self._new(auth, child_metadata)
            elif restLN != child_name or GUID == '':
                success = 'parent does not exist'
            else:
                # if everything is OK, create the new collection
                #   here GUID is of the parent collection
                success, _ = self._new(auth, child_metadata, child_name, GUID, metadata)
            response[rID] = success
        return response

    def list(self, auth, requests, neededMetadata = []):
        """ List the contents of a collection.
        
        list(requests, neededMetadata = [])
        
        requests is a dictionary with requestID as key and Logical Name as value
        neededMetadata is a list of (section, property) where property could be empty which means all properties of that section
            if neededMetadata is empty it means we need everything
        """
        auth_request = auth.get_request('read')
        # do traverse the requested Logical Names
        traverse_response = self.librarian.traverseLN(requests)
        response = {}
        for requestID, LN in requests.items():
            # for each LN
            metadata, GUID, traversedLN, restLN, wasComplete, traversedlist = traverse_response[requestID]
            if wasComplete:
                # this means the LN exists
                #decision = make_decision_metadata(metadata, auth_request)
                decision = arc.DECISION_PERMIT
                if decision != arc.DECISION_PERMIT:
                    entries = {}
                    status = 'denied'
                else:
                    # let's get the type
                    type = metadata[('entry', 'type')]
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
                        metadata = self.librarian.get(GUIDs.values(), neededMetadata)
                        # create a dictionary with the name of the entry as key and (GUID, metadata) as value
                        entries = dict([(name, (GUID, metadata[GUID])) for name, GUID in GUIDs.items()])
            else:
                entries = {}
                status = 'not found'
            response[requestID] = (entries, status)
        return response

    def move(self, auth, requests):
        """ Move a file or collection within the global namespace.
        
        move(requests)
        
        requests is a dictionary with requestID as key and
            (sourceLN, targetLN, preserverOriginal) as value
        if preserverOriginal is true this method creates a hard link instead of moving
        """
        auth_addEntry = auth.get_request('addEntry')
        auth_removeEntry = auth.get_request('removeEntry')
        traverse_request = {}
        # create a traverse request, each move request needs two traversing: source and target
        for requestID, (sourceLN, targetLN, _) in requests.items():
            # from one requestID we create two: one for the source and one for the target
            traverse_request[requestID + 'source'] = remove_trailing_slash(sourceLN)
            traverse_request[requestID + 'target'] = targetLN
        traverse_response = self.librarian.traverseLN(traverse_request)
        log.msg(arc.DEBUG, '\/\/', traverse_response)
        response = {}
        for requestID, (sourceLN, targetLN, preserveOriginal) in requests.items():
            sourceLN = remove_trailing_slash(sourceLN)
            # for each request
            log.msg(arc.DEBUG, requestID, sourceLN, targetLN, preserveOriginal)
            # get the old and the new name of the entry, these are the last elements of the Logical Names
            _, _, old_child_name = splitLN(sourceLN)
            _, _, new_child_name = splitLN(targetLN)
            # get the GUID of the source LN from the traverse response
            _, sourceGUID, _, _, sourceWasComplete, sourceTraversedList \
                = traverse_response[requestID + 'source']
            # get the GUID form the target's traverse response, this should be the parent of the target LN
            targetMetadata, targetGUID, _, targetRestLN, targetWasComplete, targetTraversedList \
                = traverse_response[requestID + 'target']
            # if the source traverse was not complete: the source LN does not exist
            if not sourceWasComplete:
                success = 'nosuchLN'
            # if the target traverse was complete: the target LN already exists
            #   but if the new_child_name was empty, then the target is considered to be a collection, so it is OK
            elif targetWasComplete and new_child_name != '':
                success = 'targetexists'
            # if the target traverse was not complete, and the non-traversed part is not just the new name
            # (which means that the parent collection does not exist)
            # or the target's traversed list is empty (which means that it has no parent: it's just a single GUID)
            elif not targetWasComplete and (targetRestLN != new_child_name or len(targetTraversedList) == 0):
                success = 'invalidtarget'
            elif sourceGUID in [guid for (_, guid) in targetTraversedList]:
                # if the the target is within the source's subtree, we cannot move it
                success = 'invalidtarget'
            else:
                # if the new child name is empty that means that the target LN has a trailing slash
                #   so we just put the old name after it
                if new_child_name == '':
                    new_child_name = old_child_name
                #decision = make_decision_metadata(targetmetadata, auth_addEntry)
                decision = arc.DECISION_PERMIT
                if decision != arc.DECISION_PERMIT:
                    success = 'adding child to parent denied'
                else:
                    log.msg(arc.DEBUG, 'adding', sourceGUID, 'to parent', targetGUID)
                    # adding the entry to the new parent
                    mm_resp = self.librarian.modifyMetadata(
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
                            # TODO: get the metadata of the parent, and decide if the user is allowed to removeEntry from it
                            log.msg(arc.DEBUG, 'removing', sourceGUID, 'from parent', source_parent_guid)
                            # delete the entry from the source parent
                            mm_resp = self.librarian.modifyMetadata(
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

    def modify(self, auth, requests):
        # TODO: auth is completely missing here
        requests, traverse_response = self._traverse(requests)
        librarian_requests = {}
        not_found = []
        for changeID, (LN, changeType, section, property, value) in requests:
            _, GUID, _, _, wasComplete, _ = traverse_response[changeID]
            if wasComplete:
                librarian_requests[changeID] = (GUID, changeType, section, property, value)
            else:
                not_found.append(changeID)
        response = self.librarian.modifyMetadata(librarian_requests)
        for changeID in not_found:
            response[changeID] = 'no such LN'
        return response

from storage.service import Service

class BartenderService(Service):

    def __init__(self, cfg):
        self.service_name = 'Bartender'

        # names of provided methods
        request_names = ['stat', 'unmakeCollection', 'makeCollection', 'list', 'move', 'putFile', 'getFile', 'addReplica', 'delFile', 'modify']
        # call the Service's constructor, 'Bartender' is the human-readable name of the service
        # request_names is the list of the names of the provided methods
        # bartender_uri is the URI of the Bartender service namespace, and 'bar' is the prefix we want to use for this namespace
        Service.__init__(self, self.service_name, request_names, 'bar', bartender_uri, cfg)
        # get the URL of the Librarian from the config file
        librarian_url = str(cfg.Get('LibrarianURL'))
        ssl_config = parse_ssl_config(cfg)
        # create a LibrarianClient from the URL
        librarian = LibrarianClient(librarian_url, ssl_config = ssl_config)
        self.bartender = Bartender(librarian, ssl_config)

    def stat(self, inpayload):
        # incoming SOAP message example:
        #
        #   <bar:stat>
        #       <bar:statRequestList>
        #           <bar:statRequestElement>
        #               <bar:requestID>0</bar:requestID>
        #               <bar:LN>/</bar:LN>
        #           </bar:statRequestElement>
        #       </bar:statRequestList>
        #   </bar:stat>
        #
        # outgoing SOAP message example:
        #
        #   <bar:statResponse>
        #       <bar:statResponseList>
        #           <bar:statResponseElement>
        #              <bar:requestID>0</bar:requestID>
        #               <bar:metadataList>
        #                   <bar:metadata>
        #                       <bar:section>states</bar:section>
        #                       <bar:property>closed</bar:property>
        #                       <bar:value>0</bar:value>
        #                   </bar:metadata>
        #                   <bar:metadata>
        #                       <bar:section>entries</bar:section>
        #                       <bar:property>testfile</bar:property>
        #                       <bar:value>cf05727b-73f3-4318-8454-16eaf10f302c</bar:value>
        #                   </bar:metadata>
        #                   <bar:metadata>
        #                       <bar:section>entry</bar:section>
        #                       <bar:property>type</bar:property>
        #                       <bar:value>collection</bar:value>
        #                   </bar:metadata>
        #               </bar:metadataList>
        #           </bar:statResponseElement>
        #       </bar:statResponseList>
        #   </bar:statResponse>

        # get all the requests
        request_nodes = get_child_nodes(inpayload.Child().Child())
        # get the requestID and LN of each request and create a dictionary where the requestID is the key and the LN is the value
        requests = dict([
            (str(request_node.Get('requestID')), str(request_node.Get('LN')))
                for request_node in request_nodes
        ])
        # call the Bartender class
        response = self.bartender.stat(inpayload.auth, requests)
        # create the metadata XML structure of each request
        for requestID, metadata in response.items():
            response[requestID] = create_metadata(metadata, 'bar')
        # create the response message with the requestID and the metadata for each request
        return create_response('bar:stat',
            ['bar:requestID', 'bar:metadataList'], response, self.newSOAPPayload(), single = True)

    def delFile(self, inpayload):
        # incoming SOAP message example:
        #
        #   <bar:delFile>
        #       <bar:delFileRequestList>
        #           <bar:delFileRequestElement>
        #               <bar:requestID>0</bar:requestID>
        #               <bar:LN>/</bar:LN>
        #           </bar:delFileRequestElement>
        #       </bar:delFileRequestList>
        #   </bar:delFile>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <bar:delFileResponse>
        #               <bar:delFileResponseList>
        #                   <bar:delFileResponseElement>
        #                       <bar:requestID>0</bar:requestID>
        #                       <bar:success>deleted</bar:success>
        #                   </bar:delFileResponseElement>
        #               </bar:delFileResponseList>
        #           </bar:delFileResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        request_nodes = get_child_nodes(inpayload.Child().Child())
        # get the requestID and LN of each request and create a dictionary where the requestID is the key and the LN is the value
        requests = dict([
            (str(request_node.Get('requestID')), str(request_node.Get('LN')))
                for request_node in request_nodes
        ])
        response = self.bartender.delFile(inpayload.auth, requests)
        return create_response('bar:delFile',
            ['bar:requestID', 'bar:success'], response, self.newSOAPPayload(), single=True)


    def getFile(self, inpayload):
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <bar:getFile>
        #           <bar:getFileRequestList>
        #               <bar:getFileRequestElement>
        #                   <bar:requestID>0</bar:requestID>
        #                   <bar:LN>/testfile</bar:LN>
        #                   <bar:protocol>byteio</bar:protocol>
        #               </bar:getFileRequestElement>
        #           </bar:getFileRequestList>
        #       </bar:getFile>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <bar:getFileResponse>
        #               <bar:getFileResponseList>
        #                   <bar:getFileResponseElement>
        #                       <bar:requestID>0</bar:requestID>
        #                       <bar:success>done</bar:success>
        #                       <bar:TURL>http://localhost:60000/byteio/12d86ba3-99d5-408e-91d1-35f7c47774e4</bar:TURL>
        #                       <bar:protocol>byteio</bar:protocol>
        #                   </bar:getFileResponseElement>
        #               </bar:getFileResponseList>
        #           </bar:getFileResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                ( # get the LN and all the protocols for each request and put them in a list
                    str(request_node.Get('LN')),
                    [str(node) for node in request_node.XPathLookup('//bar:protocol', self.ns)]
                )
            ) for request_node in request_nodes
        ])
        response = self.bartender.getFile(inpayload.auth, requests)
        return create_response('bar:getFile',
            ['bar:requestID', 'bar:success', 'bar:TURL', 'bar:protocol'], response, self.newSOAPPayload())
    
    def addReplica(self, inpayload):
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <bar:addReplica>
        #           <bar:addReplicaRequestList>
        #               <bar:putReplicaRequestElement>
        #                   <bar:requestID>0</bar:requestID>
        #                   <bar:GUID>cf05727b-73f3-4318-8454-16eaf10f302c</bar:GUID>
        #               </bar:putReplicaRequestElement>
        #           </bar:addReplicaRequestList>
        #           <bar:protocol>byteio</bar:protocol>
        #       </bar:addReplica>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <bar:addReplicaResponse>
        #               <bar:addReplicaResponseList>
        #                   <bar:addReplicaResponseElement>
        #                       <bar:requestID>0</bar:requestID>
        #                       <bar:success>done</bar:success>
        #                       <bar:TURL>http://localhost:60000/byteio/f568be18-26ae-4925-ae0c-68fe023ef1a5</bar:TURL>
        #                       <bar:protocol>byteio</bar:protocol>
        #                   </bar:addReplicaResponseElement>
        #               </bar:addReplicaResponseList>
        #           </bar:addReplicaResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>
        
        # get the list of supported protocols
        protocols = [str(node) for node in inpayload.XPathLookup('//bar:protocol', self.ns)]
        # get the GUID of each request
        request_nodes = get_child_nodes(inpayload.Child().Get('addReplicaRequestList'))
        requests = dict([(str(request_node.Get('requestID')), str(request_node.Get('GUID')))
                for request_node in request_nodes])
        response = self.bartender.addReplica(inpayload.auth, requests, protocols)
        return create_response('bar:addReplica',
            ['bar:requestID', 'bar:success', 'bar:TURL', 'bar:protocol'], response, self.newSOAPPayload())
    
    def putFile(self, inpayload):
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <bar:putFile>
        #           <bar:putFileRequestList>
        #               <bar:putFileRequestElement>
        #                   <bar:requestID>0</bar:requestID>
        #                   <bar:LN>/testfile2</bar:LN>
        #                   <bar:metadataList>
        #                       <bar:metadata>
        #                           <bar:section>states</bar:section>
        #                           <bar:property>neededReplicas</bar:property>
        #                           <bar:value>2</bar:value>
        #                       </bar:metadata>
        #                       <bar:metadata>
        #                           <bar:section>states</bar:section>
        #                           <bar:property>size</bar:property>
        #                           <bar:value>11</bar:value>
        #                       </bar:metadata>
        #                   </bar:metadataList>
        #                   <bar:protocol>byteio</bar:protocol>
        #               </bar:putFileRequestElement>
        #           </bar:putFileRequestList>
        #       </bar:putFile>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <bar:putFileResponse>
        #               <bar:putFileResponseList>
        #                   <bar:putFileResponseElement>
        #                       <bar:requestID>0</bar:requestID>
        #                       <bar:success>done</bar:success>
        #                       <bar:TURL>http://localhost:60000/byteio/b8f2987b-a718-47b3-82bb-e838470b7e00</bar:TURL>
        #                       <bar:protocol>byteio</bar:protocol>
        #                   </bar:putFileResponseElement>
        #               </bar:putFileResponseList>
        #           </bar:putFileResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (
                str(request_node.Get('requestID')), 
                (
                    str(request_node.Get('LN')),
                    parse_metadata(request_node.Get('metadataList')),
                    [str(node) for node in request_node.XPathLookup('//bar:protocol', self.ns)]
                )
            ) for request_node in request_nodes
        ])
        response = self.bartender.putFile(inpayload.auth, requests)
        return create_response('bar:putFile',
            ['bar:requestID', 'bar:success', 'bar:TURL', 'bar:protocol'], response, self.newSOAPPayload())

    def unmakeCollection(self, inpayload):
        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([(
                str(request_node.Get('requestID')),
                [str(request_node.Get('LN'))]
            ) for request_node in request_nodes
        ])
        response = self.bartender.unmakeCollection(inpayload.auth, requests)
        return create_response('bar:unmakeCollection',
            ['bar:requestID', 'bar:success'], response, self.newSOAPPayload(), single = True)

    def makeCollection(self, inpayload):
        # incoming SOAP message example:
        # 
        #   <soap-env:Body>
        #       <bar:makeCollection>
        #           <bar:makeCollectionRequestList>
        #               <bar:makeCollectionRequestElement>
        #                   <bar:requestID>0</bar:requestID>
        #                   <bar:LN>/testdir</bar:LN>
        #                   <bar:metadataList>
        #                       <bar:metadata>
        #                           <bar:section>states</bar:section>
        #                           <bar:property>closed</bar:property>
        #                           <bar:value>0</bar:value>
        #                       </bar:metadata>
        #                   </bar:metadataList>
        #               </bar:makeCollectionRequestElement>
        #           </bar:makeCollectionRequestList>
        #       </bar:makeCollection>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <bar:makeCollectionResponse>
        #               <bar:makeCollectionResponseList>
        #                   <bar:makeCollectionResponseElement>
        #                       <bar:requestID>0</bar:requestID>
        #                       <bar:success>done</bar:success>
        #                   </bar:makeCollectionResponseElement>
        #               </bar:makeCollectionResponseList>
        #           </bar:makeCollectionResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        request_nodes = get_child_nodes(inpayload.Child().Child())
        requests = dict([
            (str(request_node.Get('requestID')), 
                (str(request_node.Get('LN')), parse_metadata(request_node.Get('metadataList')))
            ) for request_node in request_nodes
        ])
        response = self.bartender.makeCollection(inpayload.auth, requests)
        return create_response('bar:makeCollection',
            ['bar:requestID', 'bar:success'], response, self.newSOAPPayload(), single = True)

    def list(self, inpayload):
        # incoming SOAP message example:
        #
        #   <soap-env:Body>
        #       <bar:list>
        #           <bar:listRequestList>
        #               <bar:listRequestElement>
        #                   <bar:requestID>0</bar:requestID>
        #                   <bar:LN>/</bar:LN>
        #               </bar:listRequestElement>
        #           </bar:listRequestList>
        #           <bar:neededMetadataList>
        #               <bar:neededMetadataElement>
        #                   <bar:section>entry</bar:section>
        #                   <bar:property></bar:property>
        #               </bar:neededMetadataElement>
        #           </bar:neededMetadataList>
        #       </bar:list>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <bar:listResponse>
        #               <bar:listResponseList>
        #                   <bar:listResponseElement>
        #                       <bar:requestID>0</bar:requestID>
        #                       <bar:entries>
        #                           <bar:entry>
        #                               <bar:name>testfile</bar:name>
        #                               <bar:GUID>cf05727b-73f3-4318-8454-16eaf10f302c</bar:GUID>
        #                               <bar:metadataList>
        #                                   <bar:metadata>
        #                                       <bar:section>entry</bar:section>
        #                                       <bar:property>type</bar:property>
        #                                       <bar:value>file</bar:value>
        #                                   </bar:metadata>
        #                               </bar:metadataList>
        #                           </bar:entry>
        #                           <bar:entry>
        #                               <bar:name>testdir</bar:name>
        #                               <bar:GUID>4cabc8cb-599d-488c-a253-165f71d4e180</bar:GUID>
        #                               <bar:metadataList>
        #                                   <bar:metadata>
        #                                       <bar:section>entry</bar:section>
        #                                       <bar:property>type</bar:property>
        #                                       <bar:value>collection</bar:value>
        #                                   </bar:metadata>
        #                               </bar:metadataList>
        #                           </bar:entry>
        #                       </bar:entries>
        #                       <bar:status>found</bar:status>
        #                   </bar:listResponseElement>
        #               </bar:listResponseList>
        #           </bar:listResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>
        
        #if inpayload.auth:
        #    print 'Bartender auth "list": ', inpayload.auth
        requests = parse_node(inpayload.Child().Get('listRequestList'),
            ['requestID', 'LN'], single = True)
        neededMetadata = [
            node_to_data(node, ['section', 'property'], single = True)
                for node in get_child_nodes(inpayload.Child().Get('neededMetadataList'))
        ]
        response0 = self.bartender.list(inpayload.auth, requests, neededMetadata)
        response = dict([
            (requestID,
            ([('bar:entry', [
                ('bar:name', name),
                ('bar:GUID', GUID),
                ('bar:metadataList', create_metadata(metadata))
            ]) for name, (GUID, metadata) in entries.items()],
            status)
        ) for requestID, (entries, status) in response0.items()])
        return create_response('bar:list',
            ['bar:requestID', 'bar:entries', 'bar:status'], response, self.newSOAPPayload())

    def move(self, inpayload):
        # incoming SOAP message example:
        #   <soap-env:Body>
        #       <bar:move>
        #           <bar:moveRequestList>
        #               <bar:moveRequestElement>
        #                   <bar:requestID>0</bar:requestID>
        #                   <bar:sourceLN>/testfile2</bar:sourceLN>
        #                   <bar:targetLN>/testdir/</bar:targetLN>
        #                   <bar:preserveOriginal>0</bar:preserveOriginal>
        #               </bar:moveRequestElement>
        #           </bar:moveRequestList>
        #       </bar:move>
        #   </soap-env:Body>
        #
        # outgoing SOAP message example:
        #
        #   <soap-env:Envelope>
        #       <soap-env:Body>
        #           <bar:moveResponse>
        #               <bar:moveResponseList>
        #                   <bar:moveResponseElement>
        #                       <bar:requestID>0</bar:requestID>
        #                       <bar:status>moved</bar:status>
        #                   </bar:moveResponseElement>
        #               </bar:moveResponseList>
        #           </bar:moveResponse>
        #       </soap-env:Body>
        #   </soap-env:Envelope>

        requests = parse_node(inpayload.Child().Child(),
            ['requestID', 'sourceLN', 'targetLN', 'preserveOriginal'])
        response = self.bartender.move(inpayload.auth, requests)
        return create_response('bar:move',
            ['bar:requestID', 'bar:status'], response, self.newSOAPPayload(), single = True)

    def modify(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['bar:changeID',
            'bar:LN', 'bar:changeType', 'bar:section', 'bar:property', 'bar:value'])
        response = self.bartender.modify(inpayload.auth, requests)
        return create_response('bar:modify', ['bar:changeID', 'bar:success'],
            response, self.newSOAPPayload(), single = True)
