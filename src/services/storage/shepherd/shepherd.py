import urlparse
import httplib
import time
import traceback
import threading
import random

import arc
from storage.xmltree import XMLTree
from storage.common import shepherd_uri, import_class_from_string, get_child_nodes, mkuid, parse_node, create_response, true, common_supported_protocols, parse_ssl_config
from storage.client import LibrarianClient, BartenderClient

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.Shepherd'))

ALIVE = 'alive'
CREATING = 'creating'
INVALID = 'invalid'
DELETED = 'deleted'
THIRDWHEEL = 'thirdwheel'

class Shepherd:

    def __init__(self, cfg):
        try:
            ssl_config = parse_ssl_config(cfg)
        except:
            log.msg(arc.ERROR, 'Error parsing client SSL config')
        try:
            backendclass = str(cfg.Get('BackendClass'))
            backendcfg = cfg.Get('BackendCfg')
            self.backend = import_class_from_string(backendclass)(backendcfg, shepherd_uri, self._file_arrived, ssl_config)
        except:
            log.msg(arc.DEBUG, 'Cannot import backend class', backendclass)
            raise
        try:
            storeclass = str(cfg.Get('StoreClass'))
            storecfg = cfg.Get('StoreCfg')
            self.store = import_class_from_string(storeclass)(storecfg)
        except:
            log.msg(arc.DEBUG, 'Cannot import store class', storeclass)
            raise
        try:
            librarianURL = str(cfg.Get('LibrarianURL'))
            self.librarian = LibrarianClient(librarianURL, ssl_config = ssl_config)
            bartenderURL = str(cfg.Get('BartenderURL'))
            self.bartender = BartenderClient(bartenderURL, ssl_config = ssl_config)
            self.serviceID = str(cfg.Get('ServiceID'))
        except:
            log.msg(arc.DEBUG, 'Cannot get LibrarianURL, BartenderURL or serviceID')
            raise
        try:
            self.period = float(str(cfg.Get('CheckPeriod')))
            self.min_interval = float(str(cfg.Get('MinCheckInterval')))
        except:
            log.msg(arc.DEBUG, 'Cannot set CheckPeriod, MinCheckInterval')
            raise
        self.changed_states = self.store.list()
        threading.Thread(target = self.checkingThread, args = [self.period]).start()
        self.doReporting = True
        threading.Thread(target = self.reportingThread, args = []).start()
        
    def reportingThread(self):
        # at the first start just wait for a few seconds
        time.sleep(5)
        while True:
            # do this forever
            try:
                # if reporting is on
                if self.doReporting:
                    # this list will hold the list of changed files we want to report
                    filelist = []
                    # when the state of a file changed somewhere the file is appended to the global 'changed_states' list
                    # so this list contains the files which state is changed between the last and the current cycle
                    while len(self.changed_states) > 0: # while there is changed file
                        # get the first one. the changed_states list contains referenceIDs
                        changed = self.changed_states.pop()
                        # get its local data (GUID, size, state, etc.)
                        localData = self.store.get(changed)
                        if not localData.has_key('GUID'):
                            log.msg(arc.DEBUG, 'Error in shepherd.reportingThread()\n\treferenceID is in changed_states, but not in store')
                        else:
                            # add to the filelist the GUID, the referenceID and the state of the file
                            filelist.append((localData.get('GUID'), changed, localData.get('state')))
                    #print 'reporting', self.serviceID, filelist
                    # call the report method of the librarian with the collected filelist and with our serviceID
                    try:
                        next_report = self.librarian.report(self.serviceID, filelist)
                    except:
                        log.msg()
                        # if next_report is below zero, then we will send everything again
                        next_report = -1
                    # we should get the time of the next report
                    # if we don't get any time, do the next report 10 seconds later
                    if not next_report:
                        next_report = 10
                    last_report = time.time()
                    # if the next report time is below zero it means:
                    if next_report < 0: # 'please send all'
                        log.msg(arc.DEBUG, '\nreporting - asked to send all file data again')
                        # add the full list of stored files to the changed_state list - all the files will be reported next time (which is immediately, see below)
                        self.changed_states.extend(self.store.list())
                    # let's wait until there is any changed file or the reporting time is up - we need to do report even if no file changed (as a heartbeat)
                    time.sleep(1)
                    while len(self.changed_states) == 0 and last_report + next_report * 0.5 > time.time():
                        time.sleep(1)
                else:
                    time.sleep(10)
            except:
                log.msg()
                time.sleep(10)
        
    def toggleReport(self, doReporting):
        self.doReporting = doReporting
        return str(self.doReporting)
    
    def _checking_checksum(self, referenceID):
        #print 'Checking checksum', referenceID
        # get the local data of this file
        localData = self.store.get(referenceID)
        # ask the backend to create the checksum of the file 
        try:
            current_checksum = self.backend.checksum(localData['localID'], localData['checksumType'])
        except:
            current_checksum = None
        # get the original checksum
        # TODO: maybe we should use the checksum from the Librarian and not the locally stored one?
        #     what if this shepherd was offline for a long time, and when it goes online again
        #     the file with this GUID is changed somehow on the other Shepherds, so the checksum is different in the Librarian
        #     but this still thinks that the file is OK according to the old checksum, and reports that
        #     is it a scenario which could happen?
        checksum = localData['checksum']
        # get the current state (which is stored locally) or an empty string if it somehow has no state
        state = localData.get('state','')
        #print '-=-', referenceID, state, checksum, current_checksum
        if checksum == current_checksum:
            # if the original and the current checksum is the same, then the replica is valid
            if state == INVALID or state == CREATING:
                # if it is currently INVALID or CREATING its state should be changed
                log.msg(arc.DEBUG, '\nCHECKSUM OK', referenceID)
                self.changeState(referenceID, ALIVE)
                state = ALIVE
            # now the state of the file is ALIVE, let's return it with the GUID and the localID (which will be needed later by checkingThread )
            return state, localData['GUID'], localData['localID']
        else:
            # or if the checksum is not the same - we have a corrupt file, or a not-fully-uploaded one
            if state == CREATING:
                # if the file's local state is CREATING, that's OK, the file is still being uploaded
                return CREATING, localData['GUID'], localData['localID']
            if state == DELETED:
                # if the file is DELETED we don't care if the checksum is wrong
                return DELETED, localData['GUID'], localData['localID']
            if state != INVALID:
                # but if it is not INVALID, not CREATING and not DELETED - so it's ALIVE: its state should be changed to INVALID
                log.msg(arc.DEBUG, '\nCHECKSUM MISMATCH', referenceID, 'original:', checksum, 'current:', current_checksum)
                self.changeState(referenceID, INVALID)
            return INVALID, localData['GUID'], localData['localID']
        
    def _file_arrived(self, referenceID):
        # this is called usually by the backend when a file arrived (gets fully uploaded)
        # call the checksum checker which will change the state to ALIVE if its checksum is OK, but leave it as CREATING if the checksum is wrong
        # TODO: either this checking should be in seperate thread, or the backend's should call this in a seperate thread?
        state, _, _ = self._checking_checksum(referenceID)
        # if _checking_checksum haven't change the state to ALIVE: the file is corrupt
        if state == CREATING:
            self.changeState(referenceID, INVALID)
        
    def checkingThread(self, period):
        # first just wait a few seconds
        time.sleep(10)
        while True:
            # do this forever
            try:
                # get the referenceIDs of all the stored files
                referenceIDs = self.store.list()
                # count them
                number = len(referenceIDs)
                # if there are stored files at all
                if number > 0:
                    # we should check all the files periodically, with a given period, which determines the checking interval for one file
                    interval = period / number
                    # but we don't want to run constantly, after a file is checked we should wait at least a specified amount of time
                    if interval < self.min_interval:
                        interval = self.min_interval
                    log.msg(arc.DEBUG,'\n', self.serviceID, 'is checking', number, 'files with interval', interval)
                    # randomize the list of files to be checked
                    random.shuffle(referenceIDs)
                    # start checking the first one
                    for referenceID in referenceIDs:
                        try:
                            # check the checksum: if the checksum is OK or not, it changes the state of the replica as well
                            # and it returns the state and the GUID and the localID of the file
                            #   if _checking_checksum changed the state then the new state is returned here:
                            state, GUID, localID = self._checking_checksum(referenceID)
                            # now the file's state is according to its checksum
                            # if it is CREATING or ALIVE:
                            if state == CREATING or state == ALIVE:
                                # first we get the file's metadata from the librarian
                                metadata = self.librarian.get([GUID])[GUID]
                                # if this metadata is not a valid file then the file must be already removed
                                if metadata.get(('entry', 'type'), '') != 'file':
                                    # it seems this is not a real file anymore
                                    # we should remove it
                                    bsuccess = self.backend.remove(localID)
                                    self.store.set(referenceID, None)
                                # if the file is ALIVE (which means it is not CREATING or DELETED)
                                if state == ALIVE:
                                    # check the number of needed replicasa
                                    needed_replicas = int(metadata.get(('states','neededReplicas'),-1))
                                    # and the number of alive replicas
                                    alive_replicas = len([property for (section, property), value in metadata.items()
                                                              if section == 'locations' and value == ALIVE])
                                    if alive_replicas < needed_replicas:
                                        # if the file has fewer replicas than needed
                                        log.msg(arc.DEBUG, '\n\nFile', GUID, 'has fewer replicas than needed.')
                                        # we offer our copy to replication
                                        response = self.bartender.addReplica({'checkingThread' : GUID}, common_supported_protocols)
                                        success, turl, protocol = response['checkingThread']
                                        #print 'addReplica response', success, turl, protocol
                                        if success == 'done':
                                            # if it's OK, we asks the backend to upload our copy to the TURL we got from the bartender
                                            self.backend.copyTo(localID, turl, protocol)
                                            # TODO: this should be done in some other thread
                                        else:
                                            log.msg(arc.DEBUG, 'checkingThread error, bartender responded', success)
                                    elif alive_replicas > needed_replicas:
                                        log.msg(arc.DEBUG, '\n\nFile', GUID, 'has %d more replicas than needed.'%(alive_replicas-needed_replicas))
                                        self.changeState(referenceID, THIRDWHEEL)
                            # or if this replica is not needed
                            elif state == THIRDWHEEL:
                                # first we get the file's metadata from the librarian
                                metadata = self.librarian.get([GUID])[GUID]
                                # get the number of THIRDWHEELS not on this Shepherd (self.serviceID)
                                thirdwheels = len([property for (section, property), value in metadata.items()
                                                   if section == 'locations' and value == THIRDWHEEL and not property.startswith(self.serviceID)])
                                # if no-one else have a thirdwheel replica, we delete this replica
                                if thirdwheels == 0:
                                    #bsuccess = self.backend.remove(localID)
                                    #self.store.set(referenceID, None)
                                    self.changeState(referenceID, DELETED)
                                # else we sheepishly set the state back to ALIVE
                                else:
                                    self.changeState(referenceID, ALIVE)
                                    state = ALIVE
                            # or if this replica is INVALID
                            elif state == INVALID:
                                log.msg(arc.DEBUG, '\n\nI have an invalid replica of file', GUID)
                                # we try to get a valid one by simply downloading this file
                                response = self.bartender.getFile({'checkingThread' : (GUID, common_supported_protocols)})
                                success, turl, protocol = response['checkingThread']
                                if success == 'done':
                                    # if it's OK, then we change the state of our replica to CREATING
                                    self.changeState(referenceID, CREATING)
                                    # then asks the backend to get the file from the TURL we got from the bartender
                                    self.backend.copyFrom(localID, turl, protocol)
                                    # and after this copying is done, we indicate that it's arrived
                                    self._file_arrived(referenceID)
                                    # TODO: this should be done in some other thread
                                else:
                                    log.msg(arc.DEBUG, 'checkingThread error, bartender responded', success)
                            if state == DELETED:
                                # remove replica if marked it as deleted
                                bsuccess = self.backend.remove(localID)
                                self.store.set(referenceID, None)
                        except:
                            log.msg(arc.DEBUG, 'ERROR checking checksum of', referenceID)
                            log.msg()
                        time.sleep(interval)
                else:
                    time.sleep(period)
            except:
                log.msg()

    def changeState(self, referenceID, newState, onlyIf = None):
        # change the file's local state and add it to the list of changed files
        self.store.lock()
        try:
            localData = self.store.get(referenceID)
            if not localData: # this file is already removed
                self.store.unlock()
                return False
            oldState = localData['state']
            log.msg(arc.DEBUG, 'changeState', referenceID, oldState, '->', newState)
            # if a previous state is given, change only if the current state is the given state
            if onlyIf and oldState != onlyIf:
                self.store.unlock()
                return False
            localData['state'] = newState
            self.store.set(referenceID, localData)
            self.store.unlock()
            # append it to the list of changed files (these will be reported)
            self.changed_states.append(referenceID)
        except:
            log.msg()
            self.store.unlock()
            return False

    def get(self, request):
        response = {}
        for requestID, getRequestData in request.items():
            log.msg(arc.DEBUG, '\n\n', getRequestData)
            referenceID = dict(getRequestData)['referenceID']
            protocols = [value for property, value in getRequestData if property == 'protocol']
            #print 'Shepherd.get:', referenceID, protocols
            localData = self.store.get(referenceID)
            #print 'localData:', localData
            if localData.get('state', INVALID) == ALIVE:
                if localData.has_key('localID'):
                    localID = localData['localID']
                    checksum = localData['checksum']
                    checksumType = localData['checksumType']
                    protocol_match = self.backend.matchProtocols(protocols)
                    if protocol_match:
                        protocol = protocol_match[0]
                        try:
                            turl = self.backend.prepareToGet(referenceID, localID, protocol)
                            if turl:
                                response[requestID] = [('TURL', turl), ('protocol', protocol),
                                    ('checksum', localData['checksum']), ('checksumType', localData['checksumType'])]
                            else:
                                response[requestID] = [('error', 'internal error (empty TURL)')]
                        except:
                            log.msg()
                            response[requestID] = [('error', 'internal error (prepareToGet exception)')]
                    else:
                        response[requestID] = [('error', 'no supported protocol found')]
                else:
                    response[requestID] = [('error', 'no such referenceID')]
            else:
                response[requestID] = [('error', 'file is not alive')]
        return response

    def put(self, request):
        #print request
        response = {}
        for requestID, putRequestData in request.items():
            protocols = [value for property, value in putRequestData if property == 'protocol']
            protocol_match = self.backend.matchProtocols(protocols)
            if protocol_match:
                # just the first protocol
                protocol = protocol_match[0]
                acl = [value for property, value in putRequestData if property == 'acl']
                # create a dictionary from the putRequestData which contains e.g. 'size', 'GUID', 'checksum', 'checksumType'
                requestData = dict(putRequestData)
                size = requestData.get('size')
                # ask the backend if there is enough space 
                availableSpace = self.backend.getAvailableSpace()
                if availableSpace and availableSpace < size:
                    response[requestID] = [('error', 'not enough space')]
                else:
                    # create a new referenceID
                    referenceID = mkuid()
                    # ask the backend to create a local ID
                    localID = self.backend.generateLocalID()
                    # create the local data of the new file
                    file_data = {'localID' : localID,
                        'GUID' : requestData.get('GUID', None),
                        'checksum' : requestData.get('checksum', None),
                        'checksumType' : requestData.get('checksumType', None),
                        'size' : size,
                        'acl': acl,
                        'state' : CREATING} # first it has the state: CREATING
                    try:
                        # ask the backend to initiate the transfer
                        turl = self.backend.prepareToPut(referenceID, localID, protocol)
                        if turl:
                            # add the returnable data to the response dict
                            response[requestID] = [('TURL', turl), ('protocol', protocol), ('referenceID', referenceID)]
                            # store the local data
                            self.store.set(referenceID, file_data)
                            # indicate that this file is 'changed': it should be reported in the next reporting cycle (in reportingThread)
                            self.changed_states.append(referenceID)
                        else:
                            response[requestID] = [('error', 'internal error (empty TURL)')]
                    except:
                        log.msg()
                        response[requestID] = [('error', 'internal error (prepareToPut exception)')]
            else:
                response[requestID] = [('error', 'no supported protocol found')]
        return response

    def delete(self,request):
        response = {}
        for requestID, referenceID in request.items():
            localData = self.store.get(referenceID)
            try:
                # note that actual deletion is done in self.reportingThread
                self.changeState(referenceID, DELETED)
                response[requestID] = 'deleted'
            except:
                response[requestID] = 'nosuchfile'
        return response


    def stat(self, request):
        properties = ['state', 'checksumType', 'checksum', 'acl', 'size', 'GUID', 'localID']
        response = {}
        for requestID, referenceID in request.items():
            localData = self.store.get(referenceID)
            response[requestID] = [referenceID]
            for p in properties:
                response[requestID].append(localData.get(p, None))
        return response

from storage.service import Service

class ShepherdService(Service):

    def __init__(self, cfg):
        try:
            serviceID = str(cfg.Get('ServiceID')).split('/')[-1]
        except:
            serviceID = "Shepherd"
        self.service_name = serviceID
        # names of provided methods
        request_names = ['get', 'put', 'stat', 'delete', 'toggleReport']
        # create the business-logic class
        self.shepherd = Shepherd(cfg)
        # get the additional request names from the backend
        backend_request_names = self.shepherd.backend.public_request_names
        # bring the additional request methods into the namespace of this object
        for name in backend_request_names:
            if not hasattr(self, name):
                setattr(self, name, getattr(self.shepherd.backend, name))
                request_names.append(name)
        # call the Service's constructor
        Service.__init__(self, serviceID, request_names, 'she', shepherd_uri, cfg)

    def stat(self, inpayload):
        request = parse_node(inpayload.Child().Child(), ['requestID', 'referenceID'], single = True)
        response = self.shepherd.stat(request)
        #print response
        return create_response('she:stat',
            ['she:requestID', 'she:referenceID', 'she:state', 'she:checksumType', 'she:checksum', 'she:acl', 'she:size', 'she:GUID', 'she:localID'], response, self.newSOAPPayload())
    

    def delete(self, inpayload):
        request = parse_node(inpayload.Child().Child(), ['requestID', 'referenceID'], single = True)
        response = self.shepherd.delete(request)
        tree = XMLTree(from_tree = 
            ('she:deleteResponseList',[
                ('she:deleteResponseElement',[
                    ('she:requestID', requestID),
                    ('she:status', status)
                    ]) for requestID, status in response.items()
                ])
            )
        out = self.newSOAPPayload()
        response_node = out.NewChild('deleteresponse')
        tree.add_to_node(response_node)
        return out

    def _putget_in(self, putget, inpayload):
        request = dict([
            (str(node.Get('requestID')), [
                (str(n.Get('property')), str(n.Get('value')))
                    for n in get_child_nodes(node.Get(putget + 'RequestDataList'))
            ]) for node in get_child_nodes(inpayload.Child().Child())])
        return request

    def _putget_out(self, putget, response):
        #print response
        tree = XMLTree(from_tree =
            ('she:' + putget + 'ResponseList', [
                ('she:' + putget + 'ResponseElement', [
                    ('she:requestID', requestID),
                    ('she:' + putget + 'ResponseDataList', [
                        ('she:' + putget + 'ResponseDataElement', [
                            ('she:property', property),
                            ('she:value', value)
                        ]) for property, value in responseData
                    ])
                ]) for requestID, responseData in response.items()
            ])
        )
        out = self.newSOAPPayload()
        response_node = out.NewChild(putget + 'response')
        tree.add_to_node(response_node)
        return out

    def get(self, inpayload):
        # if inpayload.auth:
        #     print 'Shepherd auth "get": ', inpayload.auth
        request = self._putget_in('get', inpayload)
        response = self.shepherd.get(request)
        return self._putget_out('get', response)

    def put(self, inpayload):
        request = self._putget_in('put', inpayload)
        response = self.shepherd.put(request)
        return self._putget_out('put', response)

    def toggleReport(self, inpayload):
        doReporting = str(inpayload.Child().Get('doReporting'))
        response = self.shepherd.toggleReport(doReporting == true)
        out = self.newSOAPPayload()
        response_node = out.NewChild('lbr:toggleReportResponse')
        response_node.Set(response)
        return out
