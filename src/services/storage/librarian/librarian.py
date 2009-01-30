import urlparse
import httplib
import arc
import random
import threading
import time
from storage.xmltree import XMLTree
from storage.client import AHashClient
from storage.common import librarian_uri, global_root_guid, true, false, sestore_guid
from storage.common import get_child_nodes, node_to_data, parse_metadata, create_response, \
    create_metadata, parse_node, serialize_ids, parse_ssl_config
import traceback
import copy

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.Librarian'))

class Librarian:
    def __init__(self, cfg):
        # URL of the A-Hash
        ahash_url = str(cfg.Get('AHashURL'))
        ssl_config = parse_ssl_config(cfg)
        self.ahash = AHashClient(ahash_url, ssl_config = ssl_config)
        try:
            period = float(str(cfg.Get('CheckPeriod')))
            self.hbtimeout = float(str(cfg.Get('HeartbeatTimeout')))
        except:
            log.msg(arc.DEBUG, 'Cannot find CheckPeriod, HeartbeatTimeout in the config')
            raise
        threading.Thread(target = self.checkingThread, args = [period]).start()
        
    def checkingThread(self, period):
        time.sleep(10)
        while True:
            try:
                SEs = self.ahash.get([sestore_guid])[sestore_guid]
                #print 'registered shepherds:', SEs
                now = time.time()
                late_SEs = [serviceID for (serviceID, property), nextHeartbeat in SEs.items() if property == 'nextHeartbeat' and float(nextHeartbeat) < now and nextHeartbeat != '-1']
                #print 'late shepherds (it is %s now)' % now, late_SEs
                if late_SEs:
                    serviceGUIDs = dict([(serviceGUID, serviceID) for (serviceID, property), serviceGUID in SEs.items() if property == 'serviceGUID' and serviceID in late_SEs])
                    #print 'late shepherds serviceGUIDs', serviceGUIDs
                    filelists = self.ahash.get(serviceGUIDs.keys())
                    changes = []
                    for serviceGUID, serviceID in serviceGUIDs.items():
                        filelist = filelists[serviceGUID]
                        #print 'filelist of late shepherd', serviceID, filelist
                        changes.extend([(GUID, serviceID, referenceID, 'offline')
                            for (_, referenceID), GUID in filelist.items()])
                    change_response = self._change_states(changes)
                    for _, serviceID in serviceGUIDs.items():
                        self._set_next_heartbeat(serviceID, -1)
                time.sleep(period)
            except:
                log.msg()
                time.sleep(period)

    def _change_states(self, changes):
        # we got a list of (GUID, serviceID, referenceID, state) - where GUID is of the file,
        #   serviceID is of the shepherd, referenceID is of the replica within the shepherd, and state is of this replica
        # we need to put the serviceID, referenceID pair into one string (a location)
        with_locations = [(GUID, serialize_ids([serviceID, referenceID]), state)
            for GUID, serviceID, referenceID, state in changes]
        # we will ask the librarian for each file to modify the state of this location of this file (or add this location if it was not already there)
        # if state == 'deleted' this location will be removed
        # but only if the file itself exists
        change_request = dict([
            (location, (GUID, (state != 'deleted') and 'set' or 'unset', 'locations', location, state,
                {'only if file exists' : ('is', 'entry', 'type', 'file')}))
                    for GUID, location, state in with_locations
        ])
        #print '_change_states request', change_request
        change_response = self.ahash.change(change_request)
        #print '_change_states response', change_response
        return change_response
        
    def _set_next_heartbeat(self, serviceID, next_heartbeat):
        ahash_request = {'report' : (sestore_guid, 'set', serviceID, 'nextHeartbeat', next_heartbeat, {})}
        #print '_set_next_heartbeat request', ahash_request
        ahash_response = self.ahash.change(ahash_request)
        #print '_set_next_heartbeat response', ahash_response
        if ahash_response['report'][0] != 'set':
            log.msg(arc.DEBUG, 'ERROR setting next heartbeat time!')
    
    def report(self, serviceID, filelist):
        # we got the ID of the shepherd service, and a filelist which contains (GUID, referenceID, state) tuples
        # here we get the list of registered services from the A-Hash (stored with the GUID 'sestore_guid')
        ses = self.ahash.get([sestore_guid])[sestore_guid]
        # we get the GUID of the shepherd
        serviceGUID = ses.get((serviceID,'serviceGUID'), None)
        # or this shepherd was not registered yet
        if not serviceGUID:
            ## print 'report se is not registered yet', serviceID
            # let's create a new GUID
            serviceGUID = arc.UUID()
            # let's add the new shepherd-GUID to the list of shepherd-GUIDs but only if someone else not done that just now
            ahash_request = {'report' : (sestore_guid, 'set', serviceID, 'serviceGUID', serviceGUID, {'onlyif' : ('unset', serviceID, 'serviceGUID', '')})}
            ## print 'report ahash_request', ahash_request
            ahash_response = self.ahash.change(ahash_request)
            ## print 'report ahash_response', ahash_response
            success, unmetConditionID = ahash_response['report']
            # if there was already a GUID for this service (which should have been created after we check it first but before we tried to create a new)
            if unmetConditionID:
                # let's try to get the GUID of the shepherd once again
                ses = self.ahash.get([sestore_guid])[sestore_guid]
                serviceGUID = ses.get((serviceID, 'serviceGUID'))
        # if the next heartbeat time of this shepherd is -1 or nonexistent, we will ask it to send all the file states, not just the changed
        please_send_all = int(ses.get((serviceID, 'nextHeartbeat'), -1)) == -1
        # calculate the next heartbeat time
        next_heartbeat = str(int(time.time() + self.hbtimeout))
        # register the next heartbeat time for this shepherd
        self._set_next_heartbeat(serviceID, next_heartbeat)
        # change the states of replicas in the metadata of the files to the just now reported values
        self._change_states([(GUID, serviceID, referenceID, state) for GUID, referenceID, state in filelist])
        # get the metadata of the shepherd
        se = self.ahash.get([serviceGUID])[serviceGUID]
        ## print 'report se before:', se
        # we want to know which files this shepherd stores, that's why we collect the GUIDs and referenceIDs of all the replicas the shepherd reports
        # we store this information in the A-Hash by the GUID of this shepherd (serviceGUID)
        # so this request asks the A-Hash to store the referenceID and GUID of this file in the section called 'file' in the A-Hash entry of the Shepherd
        # but if the shepherd reports that a replica is 'deleted' then it will be removed from this list (unset)
        change_request = dict([(referenceID, (serviceGUID, (state == 'deleted') and 'unset' or 'set', 'file', referenceID, GUID, {}))
            for GUID, referenceID, state in filelist])
        ## print 'report change_request:', change_request
        change_response = self.ahash.change(change_request)
        # TODO: check the response and do something if something is wrong
        ## print 'report change_response:', change_response
        ## se = self.ahash.get([serviceGUID])[serviceGUID]
        ## print 'report se after:', se
        # if we want the shepherd to send the state of all the files, not just the changed one, we return -1 as the next heartbeat's time
        if please_send_all:
            return -1
        else:
            return int(self.hbtimeout)

    def new(self, requests):
        response = {}
	for rID, metadata in requests.items():
            #print 'Processing new request:', metadata
            try:
                type = metadata[('entry','type')]
                del metadata[('entry', 'type')]
            except:
                type = None
            if type is None:
                success = 'failed: no type given'
            else:
                try:
                    GUID = metadata[('entry','GUID')]
                except:
                    GUID = arc.UUID()
                    metadata[('entry', 'GUID')] = GUID
              
		check = self.ahash.change(
                      {'new': (GUID, 'set', 'entry', 'type', type, {'0' : ('unset','entry','type','')})}
                    )
		
                status, failedCondition = check['new']
                if status == 'set':
                    success = 'success'
                    changeID = 0
                    changes = {}
                    for ((section, property), value) in metadata.items():
                        changes[changeID] = (GUID, 'set', section, property, value, {})
                        changeID += 1
                    resp = self.ahash.change(changes)
                    for r in resp.keys():
                        if resp[r][0] != 'set':
                            success += ' (failed: %s - %s)' % (resp[r][0] + str(changes[r]))
                elif failedCondition == '0':
                    success = 'failed: entry exists'
                else:
                    success = 'failed: ' + status
            response[rID] = (GUID, success)
        return response

    def get(self, requests, neededMetadata = []):
        return self.ahash.get_tree(requests, neededMetadata)

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
                while path:
                    if path[0] in ['', '.']:
                        child_guid = guid
                        child_metadata = metadata
                    else:
                        child_guid = metadata[('entries',path[0])]
                        child_metadata = self.ahash.get([child_guid])[child_guid]
		    traversed.append(path.pop(0))
                    GUIDs.append(child_guid)
                    guid = child_guid
                    metadata = child_metadata
                return metadata
            except KeyError:
                return metadata
            except:
                log.msg()
                return {}


    def traverseLN(self, requests):
        response = {}
        for rID, LN in requests.items():
            guid0, path0 = self._parse_LN(LN)
            if not guid0:
                guid = global_root_guid
            else:
                guid = guid0
            traversed = [guid0]
            GUIDs = [guid]
            path = copy.deepcopy(path0)
            metadata0 = self.ahash.get([guid])[guid]
            if not metadata0.has_key(('entry','type')):
                response[rID] = ([], False, '', guid0, None, '/'.join(path))
            else:
                try:
                    metadata = self._traverse(guid, metadata0, path, traversed, GUIDs)
		    traversedList = [(traversed[i], GUIDs[i]) for i in range(len(traversed))]
	            wasComplete = len(path) == 0
                    traversedLN = guid0 + '/' + '/'.join(traversed[1:])
                    GUID = GUIDs[-1]
                    restLN = '/'.join(path)
                    response[rID] = (traversedList, wasComplete, traversedLN, GUID, metadata, restLN)
                except:
                    log.msg()
                    response[rID] = ([], False, '', guid0, None, '/'.join(path))
            #print '?\n? traversedList, wasComplete, traversedLN, GUID, metadata, restLN\n? ', response
	return response

    def modifyMetadata(self, requests):
        changes = {}
        for changeID, (GUID, changeType, section, property, value) in requests.items():
            if changeType in ['set', 'unset']:
                conditions = {}
            elif changeType == 'add':
                changeType = 'set'
                conditions = {'0': ('unset', section, property, '')}
            changes[changeID] = (GUID, changeType, section, property, value, conditions)
        ahash_response = self.ahash.change(changes)
        response = {}
        for changeID, (success, conditionID) in ahash_response.items():
            if success in ['set', 'unset']:
                response[changeID] = success
            elif conditionID == '0':
                response[changeID] = 'entry exists'
            else:
                response[changeID] = 'failed: ' + success
        return response

    def remove(self, requests):
        ahash_request = dict([(requestID, (GUID, 'delete', '', '', '', {}))
            for requestID, GUID in requests.items()])
        ahash_response = self.ahash.change(ahash_request)
        response = {}
        for requestID, (success, _) in ahash_response.items():
            if success == 'deleted':
                response[requestID] = 'removed'
            else:
                response[requestID] = 'failed: ' + success
        return response
    
from storage.service import Service
    
class LibrarianService(Service):
    """ LibrarianService class implementing the XML interface of the storage Librarian service. """

    def __init__(self, cfg):
        """ Constructor of the LibrarianService

        LibrarianService(cfg)

        'cfg' is an XMLNode which containes the config of this service.
        """
        self.service_name = 'Librarian'
        # init logging
        # names of provided methods
        request_names = ['new','get','traverseLN', 'modifyMetadata', 'remove', 'report']
        # call the Service's constructor
        Service.__init__(self, 'Librarian', request_names, 'lbr', librarian_uri, cfg)
        self.librarian = Librarian(cfg)
    
    def new(self, inpayload):
        requests0 = parse_node(inpayload.Child().Child(),
            ['requestID', 'metadataList'], single = True, string = False)
        requests = dict([(str(requestID), parse_metadata(metadataList)) 
            for requestID, metadataList in requests0.items()])
        resp = self.librarian.new(requests)
        return create_response('lbr:new',
            ['lbr:requestID', 'lbr:GUID', 'lbr:success'], resp, self.newSOAPPayload())

    def get(self, inpayload):
        requests = [str(node.Get('GUID')) for node in get_child_nodes(inpayload.Child().Get('getRequestList'))]
        neededMetadata = [
            node_to_data(node, ['section', 'property'], single = True)
                for node in get_child_nodes(inpayload.Child().Get('neededMetadataList'))
        ]
        tree = self.librarian.get(requests, neededMetadata)
        out = arc.PayloadSOAP(self.newSOAPPayload())
        response_node = out.NewChild('lbr:getResponse')
        tree.add_to_node(response_node)
        return out

    def traverseLN(self, inpayload):
        # if inpayload.auth:
        #     print 'Librarian auth "traverseLN": ', inpayload.auth
        requests = parse_node(inpayload.Child().Child(), ['requestID', 'LN'], single = True)
        response = self.librarian.traverseLN(requests)
        for rID, (traversedList, wasComplete, traversedLN, GUID, metadata, restLN) in response.items():
            traversedListTree = [
                ('lbr:traversedListElement', [
                    ('lbr:LNPart', LNpart),
                    ('lbr:GUID', partGUID)
                ]) for (LNpart, partGUID) in traversedList
            ]
            metadataTree = create_metadata(metadata, 'lbr')
            response[rID] = (traversedListTree, wasComplete and true or false,
                traversedLN, GUID, metadataTree, restLN)
        return create_response('lbr:traverseLN',
            ['lbr:requestID', 'lbr:traversedList', 'lbr:wasComplete',
                'lbr:traversedLN', 'lbr:GUID', 'lbr:metadataList', 'lbr:restLN'], response, self.newSOAPPayload())

    def modifyMetadata(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['lbr:changeID',
            'lbr:GUID', 'lbr:changeType', 'lbr:section', 'lbr:property', 'lbr:value'])
        response = self.librarian.modifyMetadata(requests)
        return create_response('lbr:modifyMetadata', ['lbr:changeID', 'lbr:success'],
            response, self.newSOAPPayload(), single = True)

    def remove(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['lbr:requestID', 'lbr:GUID'], single = True)
        response = self.librarian.remove(requests)
        return create_response('lbr:remove', ['lbr:requestID', 'lbr:success'],
            response, self.newSOAPPayload(), single = True)
    
    def report(self, inpayload):
        request_node = inpayload.Child()
        serviceID = str(request_node.Get('serviceID'))
        filelist_node = request_node.Get('filelist')
        file_nodes = get_child_nodes(filelist_node)
        filelist = [(str(node.Get('GUID')), str(node.Get('referenceID')), str(node.Get('state'))) for node in file_nodes]
        nextReportTime = self.librarian.report(serviceID, filelist)
        out = self.newSOAPPayload()
        response_node = out.NewChild('lbr:registerResponse')
        response_node.NewChild('lbr:nextReportTime').Set(str(nextReportTime))
        return out

