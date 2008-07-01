import urlparse
import httplib
import arc
import random
import threading
import time
from storage.xmltree import XMLTree
from storage.client import AHashClient
from storage.common import catalog_uri, global_root_guid, true, false, sestore_guid
from storage.common import get_child_nodes, node_to_data, mkuid, parse_metadata, create_response, \
    create_metadata, parse_node, serialize_ids
import traceback
import copy


class Catalog:
    def __init__(self, cfg, log):
        self.log = log
        # URL of the A-Hash
        ahash_url = str(cfg.Get('AHashURL'))
        self.ahash = AHashClient(ahash_url)
        try:
            period = float(str(cfg.Get('CheckPeriod')))
            self.hbtimeout = float(str(cfg.Get('HeartbeatTimeout')))
        except:
            self.log('DEBUG', 'Cannot find CheckPeriod, HeartbeatTimeout in the config')
            raise
        threading.Thread(target = self.checkingThread, args = [period]).start()
        
    def checkingThread(self, period):
        time.sleep(10)
        while True:
            try:
                SEs = self.ahash.get([sestore_guid])[sestore_guid]
                #print 'registered storage elements:', SEs
                now = time.time()
                late_SEs = [serviceID for (serviceID, property), nextHeartbeat in SEs.items() if property == 'nextHeartbeat' and float(nextHeartbeat) < now and nextHeartbeat != '-1']
                #print 'late storage elements (it is %s now)' % now, late_SEs
                if late_SEs:
                    serviceGUIDs = dict([(serviceGUID, serviceID) for (serviceID, property), serviceGUID in SEs.items() if property == 'serviceGUID' and serviceID in late_SEs])
                    #print 'late storage elements serviceGUIDs', serviceGUIDs
                    filelists = self.ahash.get(serviceGUIDs.keys())
                    changes = []
                    for serviceGUID, serviceID in serviceGUIDs.items():
                        filelist = filelists[serviceGUID]
                        #print 'filelist of late storage element', serviceID, filelist
                        changes.extend([(GUID, serviceID, referenceID, 'offline')
                            for (_, referenceID), GUID in filelist.items()])
                    change_response = self._change_states(changes)
                    for _, serviceID in serviceGUIDs.items():
                        self._set_next_heartbeat(serviceID, -1)
                time.sleep(period)
            except:
                self.log()
                time.sleep(period)

    def _change_states(self, changes):
        with_locations = [(GUID, serialize_ids([serviceID, referenceID]), state)
            for GUID, serviceID, referenceID, state in changes]
        change_request = dict([
            (location, (GUID, 'set', 'locations', location, state,
                {'only if file exists' : ('is', 'catalog', 'type', 'file')}))
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
            self.log('DEBUG', 'ERROR setting next heartbeat time!')
    
    def report(self, serviceID, filelist):
        ses = self.ahash.get([sestore_guid])[sestore_guid]
        serviceID = str(serviceID)
        serviceGUID = ses.get((serviceID,'serviceGUID'), None)
        if not serviceGUID:
            #print 'report se is not registered yet', serviceID
            serviceGUID = mkuid()
            ahash_request = {'report' : (sestore_guid, 'set', serviceID, 'serviceGUID', serviceGUID, {'onlyif' : ('unset', serviceID, 'serviceGUID', '')})}
            #print 'report ahash_request', ahash_request
            ahash_response = self.ahash.change(ahash_request)
            #print 'report ahash_response', ahash_response
            success, unmetConditionID = ahash_response['report']
            if unmetConditionID:
                ses = self.ahash.get([sestore_guid])[sestore_guid]
                serviceGUID = ses.get((serviceID, 'serviceGUID'))
        please_send_all = int(ses.get((serviceID, 'nextHeartbeat'), -1)) == -1
        next_heartbeat = str(int(time.time() + self.hbtimeout))
        self._set_next_heartbeat(serviceID, next_heartbeat)
        self._change_states([(GUID, serviceID, referenceID, state) for GUID, referenceID, state in filelist])
        se = self.ahash.get([serviceGUID])[serviceGUID]
        #print 'report se before:', se
        change_request = dict([(referenceID, (serviceGUID, (state=='deleted') and 'unset' or 'set', 'file', referenceID, GUID, {}))
            for GUID, referenceID, state in filelist])
        #print 'report change_request:', change_request
        change_response = self.ahash.change(change_request)
        #print 'report change_response:', change_response
        se = self.ahash.get([serviceGUID])[serviceGUID]
        #print 'report se after:', se
        if please_send_all:
            return -1
        else:
            return int(self.hbtimeout)

    def new(self, requests):
        response = {}
        #print requests
        for rID, metadata in requests.items():
            #print 'Processing new request:', metadata
            try:
                type = metadata[('catalog','type')]
                del metadata[('catalog', 'type')]
            except:
                type = None
            if type is None:
                success = 'failed: no type given'
            else:
                try:
                    GUID = metadata[('catalog','GUID')]
                    del metadata[('catalog','GUID')]
                except:
                    GUID = mkuid()
                check = self.ahash.change(
                    {'new': (GUID, 'set', 'catalog', 'type', type, {'0' : ('unset','catalog','type','')})}
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
                if path:
                    if path[0] in ['', '.']:
                        child_guid = guid
                        child_metadata = metadata
                    else:
                        child_guid = metadata[('entries',path[0])]
                        child_metadata = self.ahash.get([child_guid])[child_guid]
                    traversed.append(path.pop(0))
                    GUIDs.append(child_guid)
                    return self._traverse(child_guid, child_metadata, path, traversed, GUIDs)
                else:
                    return metadata
            except KeyError:
                return metadata
            except:
                self.log()
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
            if not metadata0.has_key(('catalog','type')):
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
                    self.log()
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
    
class CatalogService(Service):
    """ CatalogService class implementing the XML interface of the storage Catalog service. """

    def __init__(self, cfg):
        """ Constructor of the CatalogService

        CatalogService(cfg)

        'cfg' is an XMLNode which containes the config of this service.
        """
        # names of provided methods
        request_names = ['new','get','traverseLN', 'modifyMetadata', 'remove', 'report']
        # call the Service's constructor
        Service.__init__(self, 'Catalog', request_names, 'cat', catalog_uri, cfg)
        self.catalog = Catalog(cfg, self.log)
    
    def new(self, inpayload):
        requests0 = parse_node(inpayload.Child().Child(),
            ['requestID', 'metadataList'], single = True, string = False)
        requests = dict([(str(requestID), parse_metadata(metadataList))
            for requestID, metadataList in requests0.items()])
        resp = self.catalog.new(requests)
        return create_response('cat:new',
            ['cat:requestID', 'cat:GUID', 'cat:success'], resp, self.newSOAPPayload())

    def get(self, inpayload):
        requests = [str(node.Get('GUID')) for node in get_child_nodes(inpayload.Child().Get('getRequestList'))]
        neededMetadata = [
            node_to_data(node, ['section', 'property'], single = True)
                for node in get_child_nodes(inpayload.Child().Get('neededMetadataList'))
        ]
        tree = self.catalog.get(requests, neededMetadata)
        out = arc.PayloadSOAP(self.newSOAPPayload())
        response_node = out.NewChild('cat:getResponse')
        tree.add_to_node(response_node)
        return out

    def traverseLN(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['requestID', 'LN'], single = True)
        response = self.catalog.traverseLN(requests)
        for rID, (traversedList, wasComplete, traversedLN, GUID, metadata, restLN) in response.items():
            traversedListTree = [
                ('cat:traversedListElement', [
                    ('cat:LNPart', LNpart),
                    ('cat:GUID', partGUID)
                ]) for (LNpart, partGUID) in traversedList
            ]
            metadataTree = create_metadata(metadata, 'cat')
            response[rID] = (traversedListTree, wasComplete and true or false,
                traversedLN, GUID, metadataTree, restLN)
        return create_response('cat:traverseLN',
            ['cat:requestID', 'cat:traversedList', 'cat:wasComplete',
                'cat:traversedLN', 'cat:GUID', 'cat:metadataList', 'cat:restLN'], response, self.newSOAPPayload())

    def modifyMetadata(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['cat:changeID',
            'cat:GUID', 'cat:changeType', 'cat:section', 'cat:property', 'cat:value'])
        response = self.catalog.modifyMetadata(requests)
        return create_response('cat:modifyMetadata', ['cat:changeID', 'cat:success'],
            response, self.newSOAPPayload(), single = True)

    def remove(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['cat:requestID', 'cat:GUID'], single = True)
        response = self.catalog.remove(requests)
        return create_response('cat:remove', ['cat:requestID', 'cat:success'],
            response, self.newSOAPPayload(), single = True)
    
    def report(self, inpayload):
        request_node = inpayload.Child()
        serviceID = str(request_node.Get('serviceID'))
        filelist_node = request_node.Get('filelist')
        file_nodes = get_child_nodes(filelist_node)
        filelist = [(str(node.Get('GUID')), str(node.Get('referenceID')), str(node.Get('state'))) for node in file_nodes]
        nextReportTime = self.catalog.report(serviceID, filelist)
        out = self.newSOAPPayload()
        response_node = out.NewChild('cat:registerResponse')
        response_node.NewChild('cat:nextReportTime').Set(str(nextReportTime))
        return out

