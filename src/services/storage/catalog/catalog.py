import urlparse
import httplib
import arc
import random
import threading
import time
from storage.xmltree import XMLTree
from storage.client import HashClient
from storage.common import catalog_uri, global_root_guid, true, false
from storage.common import get_child_nodes, node_to_data, mkuid, parse_metadata, create_response, \
    create_metadata, parse_node, serialize_ids, parse_ids
import traceback
import copy

sestore_guid = '1'

class Catalog:
    def __init__(self, cfg):
        # URL of the Hash
        hash_url = str(cfg.Get('HashURL'))
        self.hash = HashClient(hash_url)
        try:
            period = float(str(cfg.Get('CheckPeriod')))
            self.hbtimeout = float(str(cfg.Get('HeartbeatTimeout')))
        except:
            print 'Cannot find CheckPeriod, HeartbeatTimeout in the config'
            raise
        threading.Thread(target = self.checkingThread, args = [period]).start()
        
    def checkingThread(self, period):
        time.sleep(10)
        while True:
            try:
                SEs = self.hash.get([sestore_guid])[sestore_guid]
                print 'registered storage elements:', SEs
                now = time.time()
                late_SEs = [serviceID for (serviceID, property), nextHeartbeat in SEs.items() if property == 'nextHeartbeat' and float(nextHeartbeat) < now and nextHeartbeat != '-1']
                print 'late storage elements (it is %s now)' % now, late_SEs
                if late_SEs:
                    serviceGUIDs = dict([(serviceGUID, serviceID) for (serviceID, property), serviceGUID in SEs.items() if property == 'serviceGUID' and serviceID in late_SEs])
                    print 'late storage elements serviceGUIDs', serviceGUIDs
                    filelists = self.hash.get(serviceGUIDs.keys())
                    changes = []
                    for serviceGUID, serviceID in serviceGUIDs.items():
                        filelist = filelists[serviceGUID]
                        print 'filelist of late storage element', serviceID, filelist
                        changes.extend([(GUID, serviceID, referenceID, 'offline')
                            for (_, referenceID), GUID in filelist.items()])
                    change_response = self._change_states(changes)
                    for _, serviceID in serviceGUIDs.items():
                        self._set_next_heartbeat(serviceID, -1)
                time.sleep(period)
            except:
                print traceback.format_exc()
                time.sleep(period)

    def _change_states(self, changes):
        with_locations = [(GUID, serialize_ids([serviceID, referenceID]), state)
            for GUID, serviceID, referenceID, state in changes]
        change_request = dict([
            (location, (GUID, 'set', 'locations', location, state,
                {'only if file exists' : ('is', 'catalog', 'type', 'file')}))
                    for GUID, location, state in with_locations
        ])
        print '_change_states request', change_request
        change_response = self.hash.change(change_request)
        print '_change_states response', change_response
        return change_response
        
    def _set_next_heartbeat(self, serviceID, next_heartbeat):
        hash_request = {'report' : (sestore_guid, 'set', serviceID, 'nextHeartbeat', next_heartbeat, {})}
        print '_set_next_heartbeat request', hash_request
        hash_response = self.hash.change(hash_request)
        print '_set_next_heartbeat response', hash_response
        if hash_response['report'][0] != 'set':
            print 'ERROR setting next heartbeat time!'
    
    def report(self, serviceID, filelist):
        ses = self.hash.get([sestore_guid])[sestore_guid]
        serviceID = str(serviceID)
        serviceGUID = ses.get((serviceID,'serviceGUID'), None)
        if not serviceGUID:
            print 'report se is not registered yet', serviceID
            serviceGUID = mkuid()
            hash_request = {'report' : (sestore_guid, 'set', serviceID, 'serviceGUID', serviceGUID, {'onlyif' : ('unset', serviceID, 'serviceGUID', '')})}
            print 'report hash_request', hash_request
            hash_response = self.hash.change(hash_request)
            print 'report hash_response', hash_response
            success, unmetConditionID = hash_response['report']
            if unmetConditionID:
                ses = self.hash.get([sestore_guid])[sestore_guid]
                serviceGUID = ses.get((serviceID, 'serviceGUID'))
        please_send_all = int(ses.get((serviceID, 'nextHeartbeat'), -1)) == -1
        next_heartbeat = str(int(time.time() + self.hbtimeout))
        self._set_next_heartbeat(serviceID, next_heartbeat)
        self._change_states([(GUID, serviceID, referenceID, state) for GUID, referenceID, state in filelist])
        se = self.hash.get([serviceGUID])[serviceGUID]
        print 'report se before:', se
        change_request = dict([(referenceID, (serviceGUID, 'set', 'file', referenceID, GUID, {}))
            for GUID, referenceID, _ in filelist])
        print 'report change_request:', change_request
        change_response = self.hash.change(change_request)
        print 'report change_response:', change_response
        se = self.hash.get([serviceGUID])[serviceGUID]
        print 'report se after:', se
        if please_send_all:
            return -1
        else:
            return int(self.hbtimeout)

    def new(self, requests):
        response = {}
        print requests
        for rID, metadata in requests.items():
            print 'Processing new request:', metadata
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
                check = self.hash.change(
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
                    resp = self.hash.change(changes)
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
        return self.hash.get_tree(requests, neededMetadata)

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
                        child_metadata = self.hash.get([child_guid])[child_guid]
                    traversed.append(path.pop(0))
                    GUIDs.append(child_guid)
                    return self._traverse(child_guid, child_metadata, path, traversed, GUIDs)
                else:
                    return metadata
            except KeyError:
                return metadata
            except:
                print traceback.format_exc()
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
            metadata0 = self.hash.get([guid])[guid]
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
                    print traceback.format_exc()
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
        hash_response = self.hash.change(changes)
        response = {}
        for changeID, (success, conditionID) in hash_response.items():
            if success in ['set', 'unset']:
                response[changeID] = success
            elif conditionID == '0':
                response[changeID] = 'entry exists'
            else:
                response[changeID] = 'failed: ' + success
        return response

    def remove(self, requests):
        hash_request = dict([(requestID, (GUID, 'delete', '', '', '', {}))
            for requestID, GUID in requests.items()])
        hash_response = self.hash.change(hash_request)
        response = {}
        for requestID, (success, _) in hash_response.items():
            if success == 'deleted':
                response[requestID] = 'removed'
            else:
                response[requestID] = 'failed: ' + success
        return response
    
class CatalogService:
    """ CatalogService class implementing the XML interface of the storage Catalog service. """

    def __init__(self, cfg):
        """ Constructor of the CatalogService

        CatalogService(cfg)

        'cfg' is an XMLNode which containes the config of this service.
        """
        print "CatalogService constructor called"
        self.catalog = Catalog(cfg)
        # set the default namespace for the Catalog service
        self.cat_ns = arc.NS({'cat':catalog_uri})

    
    def new(self, inpayload):
        requests0 = parse_node(inpayload.Child().Child(),
            ['requestID', 'metadataList'], single = True, string = False)
        requests = dict([(str(requestID), parse_metadata(metadataList))
            for requestID, metadataList in requests0.items()])
        resp = self.catalog.new(requests)
        return create_response('cat:new',
            ['cat:requestID', 'cat:GUID', 'cat:success'], resp, self.cat_ns)

    def get(self, inpayload):
        requests = [str(node.Get('GUID')) for node in get_child_nodes(inpayload.Child().Get('getRequestList'))]
        neededMetadata = [
            node_to_data(node, ['section', 'property'], single = True)
                for node in get_child_nodes(inpayload.Child().Get('neededMetadataList'))
        ]
        tree = self.catalog.get(requests, neededMetadata)
        out = arc.PayloadSOAP(self.cat_ns)
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
                'cat:traversedLN', 'cat:GUID', 'cat:metadataList', 'cat:restLN'], response, self.cat_ns)

    def modifyMetadata(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['cat:changeID',
            'cat:GUID', 'cat:changeType', 'cat:section', 'cat:property', 'cat:value'])
        response = self.catalog.modifyMetadata(requests)
        return create_response('cat:modifyMetadata', ['cat:changeID', 'cat:success'],
            response, self.cat_ns, single = True)

    def remove(self, inpayload):
        requests = parse_node(inpayload.Child().Child(), ['cat:requestID', 'cat:GUID'], single = True)
        response = self.catalog.remove(requests)
        return create_response('cat:remove', ['cat:requestID', 'cat:success'],
            response, self.cat_ns, single = True)
    
    def report(self, inpayload):
        request_node = inpayload.Child()
        serviceID = str(request_node.Get('serviceID'))
        filelist_node = request_node.Get('filelist')
        file_nodes = get_child_nodes(filelist_node)
        filelist = [(str(node.Get('GUID')), str(node.Get('referenceID')), str(node.Get('state'))) for node in file_nodes]
        nextReportTime = self.catalog.report(serviceID, filelist)
        out = arc.PayloadSOAP(self.cat_ns)
        response_node = out.NewChild('cat:registerResponse')
        response_node.NewChild('cat:nextReportTime').Set(str(nextReportTime))
        return out

    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        try:
            # gets the namespace prefix of the catalog namespace in its incoming payload
            catalog_prefix = inpayload.NamespacePrefix(catalog_uri)
            # gets the namespace prefix of the request
            request_prefix = inpayload.Child().Prefix()
            if request_prefix != catalog_prefix:
                # if the request is not in the catalog namespace
                raise Exception, 'wrong namespace (%s)' % request_prefix
            # get the name of the request without the namespace prefix
            request_name = inpayload.Child().Name()
            print '     catalog.%s called' % request_name
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
            outpayload = arc.PayloadSOAP(self.cat_ns)
            outpayload.NewChild('catalog:Fault').Set(exc)
            outmsg.Payload(outpayload)
            return arc.MCC_Status(arc.STATUS_OK)

    # names of provided methods
    request_names = ['new','get','traverseLN', 'modifyMetadata', 'remove', 'report']
