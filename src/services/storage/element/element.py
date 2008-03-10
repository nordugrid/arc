import urlparse
import httplib
import arc
from storage.xmltree import XMLTree
from storage.common import element_uri, import_class_from_string, get_child_nodes, mkuid
import traceback

class Element:

    def __init__(self, backend, store):
        self.backend = backend
        self.store = store

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
                protocol_match = self.backend.matchProtocols(protocols)
                if protocol_match:
                    protocol = protocol_match[0]
                    try:
                        turl = self.backend.prepareToGet(localID, protocol)
                        if turl:
                            response[requestID] = [('TURL', turl), ('protocol', protocol)]
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
                        'acl': acl}
                    try:
                        turl = self.backend.prepareToPut(localID, protocol)
                        if turl:
                            response[requestID] = [('TURL', turl), ('protocol', protocol), ('referenceID', referenceID)]
                            self.store.set(referenceID, file_data)
                        else:
                            response[requestID] = [('error', 'internal error (empty TURL)')]
                    except:
                        print traceback.format_exc()
                        response[requestID] = [('error', 'internal error (prepareToPut exception)')]
            else:
                response[requestID] = [('error', 'no supported protocol found')]
        return response



class ElementService:

    def __init__(self, cfg):
        print "Storage Element service constructor called"
        self.se_ns = arc.NS({'se':element_uri})
        backendclass = str(cfg.Get('BackendClass'))
        backendcfg = cfg.Get('BackendCfg')
        self.backend = import_class_from_string(backendclass)(backendcfg, element_uri)
        storeclass = str(cfg.Get('StoreClass'))
        storecfg = cfg.Get('StoreCfg')
        store = import_class_from_string(storeclass)(storecfg)
        self.element = Element(self.backend, store)

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

    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        print "Process called"
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
            if request_name not in self.request_names + self.backend.public_request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            if request_name in self.request_names:
                outpayload = getattr(self, request_name)(inpayload)
            # or if the backend has a method of this name:
            elif request_name in self.backend.public_request_names:
                outpayload = getattr(self.backend, request_name)(inpayload)
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
    request_names = ['get', 'put']
