import urlparse
import httplib
import time
import traceback
import threading
import random

import arc
from storage.xmltree import XMLTree
from storage.common import element_uri, import_class_from_string, get_child_nodes, mkuid, parse_node, create_response, true
from storage.client import CatalogClient, ManagerClient, ByteIOClient

ALIVE = 'alive'
CREATING = 'creating'
INVALID = 'invalid'
DELETED = 'deleted'

class Element:

    def __init__(self, cfg):
        try:
            backendclass = str(cfg.Get('BackendClass'))
            backendcfg = cfg.Get('BackendCfg')
            self.backend = import_class_from_string(backendclass)(backendcfg, element_uri, self._file_arrived)
        except:
            print 'Cannot import backend class', backendclass
            raise
        try:
            storeclass = str(cfg.Get('StoreClass'))
            storecfg = cfg.Get('StoreCfg')
            self.store = import_class_from_string(storeclass)(storecfg)
        except:
            print 'Cannot import store class', storeclass
            raise
        try:
            catalogURL = str(cfg.Get('CatalogURL'))
            self.catalog = CatalogClient(catalogURL)
            managerURL = str(cfg.Get('ManagerURL'))
            self.manager = ManagerClient(managerURL)
            self.serviceID = str(cfg.Get('ServiceID'))
        except:
            print 'Cannot get CatalogURL, ManagerURL or serviceID'
            raise
        try:
            self.period = float(str(cfg.Get('CheckPeriod')))
            self.min_interval = float(str(cfg.Get('MinCheckInterval')))
        except:
            print 'Cannot set CheckPeriod, MinCheckInterval'
            raise
        self.changed_states = self.store.list()
        threading.Thread(target = self.checkingThread, args = [self.period]).start()
        self.doReporting = True
        threading.Thread(target = self.reportingThread, args = []).start()
        
    def reportingThread(self):
        time.sleep(5)
        while True:
            try:
                if self.doReporting:
                    filelist = []
                    while len(self.changed_states) > 0:
                        changed = self.changed_states.pop()
                        localData = self.store.get(changed)
                        if not localData.has_key('GUID'):
                            print 'Error',changed,localData
                        else:
                            filelist.append((localData.get('GUID'), changed, localData.get('state')))
                            if localData['state']==DELETED:
                                bsuccess = self.backend.remove(localData['localID'])
                                self.store.set(changed, None)
                    print 'reporting', self.serviceID, filelist
                    next_report = self.catalog.report(self.serviceID, filelist)
                    last_report = time.time()
                    if next_report < 0: # 'please send all'
                        print '\nreporting - asked to send all file data again'
                        self.changed_states = self.store.list()
                    while len(self.changed_states) == 0 and last_report + next_report * 0.5 > time.time():
                        time.sleep(1)
                else:
                    time.sleep(10)
            except:
                traceback.print_exc()
                time.sleep(10)
        
    def toggleReport(self, doReporting):
        self.doReporting = doReporting
        return str(self.doReporting)
    
    def _checking_checksum(self, referenceID):
        #print 'Checking checksum', referenceID
        localData = self.store.get(referenceID)
        if not localData.has_key('localID'):
            return CREATING, None, None
        current_checksum = self.backend.checksum(localData['localID'], localData['checksumType'])
        checksum = localData['checksum']
        state = localData.get('state','')
        if state == CREATING:
            return CREATING, None, None 
        #print '-=-', referenceID, state, checksum, current_checksum
        if checksum == current_checksum:
            if state != ALIVE:
                print '\nCHECKSUM OK', referenceID
                self.changeState(referenceID, ALIVE)
            return ALIVE, localData['GUID'], localData['localID']
        else:
            if state != INVALID:
                print '\nCHECKSUM MISMATCH', referenceID, 'original:', checksum, 'current:', current_checksum
                self.changeState(referenceID, INVALID)
            return INVALID, localData['GUID'], localData['localID']
        
    def _file_arrived(self, referenceID):
        self.changeState(referenceID, ALIVE, onlyIf = CREATING)
        return self._checking_checksum(referenceID)
    
    def _checking_replicano(self, GUID):
        try:
            metadata = self.catalog.get([GUID])[GUID]
            needed_replicas = int(metadata.get(('states','neededReplicas'),1))
            valid_replicas = len([property for (section, property), value in metadata.items()
                              if section == 'locations' and value == ALIVE])
            return valid_replicas >= needed_replicas
        except:
            traceback.print_exc()
            return True


    def _checking_guid(self, GUID):
        try:
            metadata = self.catalog.get([GUID]).get(GUID)
            if not metadata.has_key(('states', 'neededReplicas')):
                # catalog couldn't find the file; we don't need any replicas
                return 'NotInCatalog'
        except:
            traceback.print_exc()
            return 'CatalogBorken'
        
        
    def checkingThread(self, period):
        time.sleep(10)
        while True:
            try:
                referenceIDs = self.store.list()
                number = len(referenceIDs)
                if number > 0:
                    interval = period / number
                    if interval < self.min_interval:
                        interval = self.min_interval
                    print '\nElement is checking', number, 'files with interval', interval
                    random.shuffle(referenceIDs)
                    for referenceID in referenceIDs:
                        try:
                            state, GUID, localID = self._checking_checksum(referenceID)
                            if state == CREATING or state == ALIVE:
                                if self._checking_guid(GUID) == 'NotInCatalog':
                                    # guid not in catalog, we don't need the replicas
                                    self.changeState(referenceID,DELETED)
                            elif state == ALIVE:
                                if not self._checking_replicano(GUID):
                                    print '\n\nFile', GUID, 'has fewer replicas than needed.'
                                    response = self.manager.addReplica({'checkingThread' : GUID}, ['byteio'])
                                    success, turl, protocol = response['checkingThread']
                                    print 'addReplica response', success, turl, protocol
                                    if success == 'done':
                                        self.backend.copyTo(localID, turl, protocol)
                                    else:
                                        print 'checkingThread error, manager responded', success
                            if state == INVALID:
                                print '\n\nI have an invalid replica of file', GUID
                                response = self.manager.getFile({'checkingThread' : (GUID, ['byteio'])})
                                success, turl, protocol = response['checkingThread']
                                if success == 'done':
                                    self.changeState(referenceID, CREATING)
                                    self.backend.copyFrom(localID, turl, protocol)
                                    self._file_arrived(referenceID)
                                else:
                                    print 'checkingThread error, manager responded', success
                        except:
                            print 'ERROR checking checksum of', referenceID
                            print traceback.format_exc()
                        time.sleep(interval)
                else:
                    time.sleep(period)
            except:
                print traceback.format_exc()

    def changeState(self, referenceID, newState, onlyIf = None):
        self.store.lock()
        try:
            localData = self.store.get(referenceID)
            oldState = localData['state']
            if onlyIf and oldState != onlyIf:
                self.store.unlock()
                return False
            localData['state'] = newState
            self.store.set(referenceID, localData)
            self.store.unlock()
            self.changed_states.append(referenceID)
        except:
            print traceback.format_exc()
            self.store.unlock()
            return False

    def get(self, request):
        print request
        response = {}
        for requestID, getRequestData in request.items():
            referenceID = dict(getRequestData)['referenceID']
            protocols = [value for property, value in getRequestData if property == 'protocol']
            print 'Element.get:', referenceID, protocols
            localData = self.store.get(referenceID)
            print 'localData:', localData
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
                            print traceback.format_exc()
                            response[requestID] = [('error', 'internal error (prepareToGet exception)')]
                    else:
                        response[requestID] = [('error', 'no supported protocol found')]
                else:
                    response[requestID] = [('error', 'no such referenceID')]
            else:
                response[requestID] = [('error', 'file is not alive')]
        return response

    def put(self, request):
        print request
        response = {}
        for requestID, putRequestData in request.items():
            protocols = [value for property, value in putRequestData if property == 'protocol']
            protocol_match = self.backend.matchProtocols(protocols)
            if protocol_match:
                protocol = protocol_match[0]
                acl = [value for property, value in putRequestData if property == 'acl']
                requestData = dict(putRequestData)
                size = requestData.get('size')
                availableSpace = self.backend.getAvailableSpace()
                if availableSpace and availableSpace < size:
                    response[requestID] = [('error', 'not enough space')]
                else:
                    referenceID = mkuid()
                    localID = self.backend.generateLocalID()
                    file_data = {'localID' : localID,
                        'GUID' : requestData.get('GUID', None),
                        'checksum' : requestData.get('checksum', None),
                        'checksumType' : requestData.get('checksumType', None),
                        'size' : size,
                        'acl': acl,
                        'state' : CREATING}
                    try:
                        turl = self.backend.prepareToPut(referenceID, localID, protocol)
                        if turl:
                            response[requestID] = [('TURL', turl), ('protocol', protocol), ('referenceID', referenceID)]
                            self.store.set(referenceID, file_data)
                            self.changed_states.append(referenceID)
                        else:
                            response[requestID] = [('error', 'internal error (empty TURL)')]
                    except:
                        print traceback.format_exc()
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
                self.changeState(referenceID,DELETED)
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

class ElementService(Service):

    def __init__(self, cfg):
        # names of provided methods
        request_names = ['get', 'put', 'stat', 'delete', 'toggleReport']
        # create the business-logic class
        self.element = Element(cfg)
        # get the additional request names from the backend
        backend_request_names = self.element.backend.public_request_names
        # bring the additional request methods into the namespace of this object
        for name in backend_request_names:
            if not hasattr(self, name):
                setattr(self, name, getattr(self.element.backend, name))
                request_names.append(name)
        # call the Service's constructor
        Service.__init__(self, 'Element', request_names, 'se', element_uri)


    def stat(self, inpayload):
        request = parse_node(inpayload.Child().Child(), ['requestID', 'referenceID'], single = True)
        response = self.element.stat(request)
        print response
        return create_response('se:stat',
            ['se:requestID', 'se:referenceID', 'se:state', 'se:checksumType', 'se:checksum', 'se:acl', 'se:size', 'se:GUID', 'se:localID'], response, self.newSOAPPayload())
    

    def delete(self, inpayload):
        request = parse_node(inpayload.Child().Child(), ['requestID', 'referenceID'], single = True)
        response = self.element.delete(request)
        tree = XMLTree(from_tree = 
            ('se:deleteResponseList',[
                ('se:deleteResponseElement',[
                    ('se:requestID', requestID),
                    ('se:status', status)
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
        print response
        tree = XMLTree(from_tree =
            ('se:' + putget + 'ResponseList', [
                ('se:' + putget + 'ResponseElement', [
                    ('se:requestID', requestID),
                    ('se:' + putget + 'ResponseDataList', [
                        ('se:' + putget + 'ResponseDataElement', [
                            ('se:property', property),
                            ('se:value', value)
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
        request = self._putget_in('get', inpayload)
        response = self.element.get(request)
        return self._putget_out('get', response)

    def put(self, inpayload):
        request = self._putget_in('put', inpayload)
        response = self.element.put(request)
        return self._putget_out('put', response)

    def toggleReport(self, inpayload):
        doReporting = str(inpayload.Child().Get('doReporting'))
        response = self.element.toggleReport(doReporting == true)
        out = self.newSOAPPayload()
        response_node = out.NewChild('cat:toggleReportResponse')
        response_node.Set(response)
        return out
