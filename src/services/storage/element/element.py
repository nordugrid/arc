import urlparse
import httplib
import time
import traceback
import threading
import random

import arc
from storage.xmltree import XMLTree
from storage.common import element_uri, import_class_from_string, get_child_nodes, mkuid, parse_node, create_response, true
from storage.client import CatalogClient

ALIVE = 'alive'
CREATING = 'creating'
INVALID = 'invalid'

class Element:

    def __init__(self, cfg):
        try:
            backendclass = str(cfg.Get('BackendClass'))
            backendcfg = cfg.Get('BackendCfg')
            self.backend = import_class_from_string(backendclass)(backendcfg, element_uri, self.changeState)
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
            self.serviceID = str(cfg.Get('ServiceID'))
        except:
            print 'Cannot get CatalogURL or serviceID'
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
                        filelist.append((localData['GUID'], changed, localData['state']))
                    print 'reporting', self.serviceID, filelist
                    next_report = self.catalog.report(self.serviceID, filelist)
                    if next_report > 0:
                        time.sleep(next_report * 0.9)
                    else: # 'please send all'
                        print '\nreporting - asked to send all file data again'
                        self.changed_states = self.store.list()
                else:
                    time.sleep(10)
            except:
                traceback.print_exc()
                time.sleep(10)
        
    def toggleReport(self, doReporting):
        self.doReporting = doReporting
        return str(self.doReporting)
        
    def checkingThread(self, period):
        while True:
            try:
                referenceIDs =  self.store.list()
                #print referenceIDs
                number = len(referenceIDs)
                if number > 0:
                    interval = period / number
                    if interval < self.min_interval:
                        interval = self.min_interval
                    #print 'Checking', number, 'files with interval', interval
                    random.shuffle(referenceIDs)
                    for referenceID in referenceIDs:
                        try:
                            #print 'Checking checksum', referenceID
                            localData = self.store.get(referenceID)
                            current_checksum = self.backend.checksum(localData['localID'], localData['checksumType'])
                            checksum = localData['checksum']
                            state = localData.get('state','')
                            #print '-=-', referenceID, state, checksum, current_checksum
                            if checksum == current_checksum:
                                if state != ALIVE:
                                    print 'Checksum OK', referenceID
                                    self.changeState(referenceID, ALIVE)
                            else:
                                if state != INVALID:
                                    print 'Checksum mismatch:', referenceID, 'original:', checksum, 'current:', current_checksum
                                    self.changeState(referenceID, INVALID)
                        except:
                            print 'ERROR checking checksum of', referenceID
                            print traceback.format_exc()
                        time.sleep(interval)
                else:
                    time.sleep(period)
            except:
                print traceback.format_exc()

    def changeState(self, referenceID, newState, onlyIf = None):
        print 'element.changeState locking store'
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

    def stat(self, request):
        properties = ['state', 'checksumType', 'checksum', 'acl', 'size', 'GUID', 'localID']
        response = {}
        for requestID, referenceID in request.items():
            localData = self.store.get(referenceID)
            response[requestID] = [referenceID]
            for p in properties:
                response[requestID].append(localData.get(p, None))
        return response

class ElementService:

    def __init__(self, cfg):
        print "Storage Element service constructor called"
        self.se_ns = arc.NS({'se':element_uri})
        self.element = Element(cfg)
        backend_request_names = self.element.backend.public_request_names
        for name in backend_request_names:
            if not hasattr(self, name):
                setattr(self, name, getattr(self.element.backend, name))
                self.request_names.append(name)
        #print self.request_names
        #print self.notify


    def stat(self, inpayload):
        request = parse_node(inpayload.Child().Child(), ['requestID', 'referenceID'], single = True)
        response = self.element.stat(request)
        print response
        return create_response('se:stat',
            ['se:requestID', 'se:referenceID', 'se:state', 'se:checksumType', 'se:checksum', 'se:acl', 'se:size', 'se:GUID', 'se:localID'], response, self.se_ns)

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
        out = arc.PayloadSOAP(self.se_ns)
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
        out = arc.PayloadSOAP(self.se_ns)
        response_node = out.NewChild('cat:toggleReportResponse')
        response_node.Set(response)
        return out

    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        try:
            # gets the namespace prefix of the Hash namespace in its incoming payload
            element_prefix = inpayload.NamespacePrefix(element_uri)
            request_node = inpayload.Child()
            if request_node.Prefix() != element_prefix:
                raise Exception, 'wrong namespace (%s)' % request_node.Prefix()
            # get the name of the request without the namespace prefix
            request_name = request_node.Name()
            print '     element.%s called' % request_name
            if request_name not in self.request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            if request_name in self.request_names:
                outpayload = getattr(self, request_name)(inpayload)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            exc = traceback.format_exc()
            print exc
            # set an empty outgoing payload
            outpayload = arc.PayloadSOAP(self.se_ns)
            outpayload.NewChild('se:Fault').Set(exc)
            outmsg.Payload(outpayload)
            return arc.MCC_Status(arc.STATUS_OK)

    # names of provided methods
    request_names = ['get', 'put', 'stat', 'toggleReport']
