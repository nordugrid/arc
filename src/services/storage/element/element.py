import urlparse
import httplib
import arc
from storage.xmltree import XMLTree
from storage.common import element_uri, import_class_from_string, get_child_nodes
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
            protocol_match = [protocol for protocol in protocols if protocol in self.backend.supported_protocols]
            if protocol_match:
                protocol = protocol_match[0]
                turl = self.backend.prepareToGet(referenceID, protocol)
                response[requestID] = [('TURL', turl), ('protocol', protocol)]
            else:
                raise Exception, 'No supported protocol found'
        return response

class ElementService:

    def __init__(self, cfg):
        print "Storage Element service constructor called"
        self.se_ns = arc.NS({'se':element_uri})
        backendclass = str(cfg.Get('BackendClass'))
        backendcfg = cfg.Get('BackendCfg')
        self.backend = import_class_from_string(backendclass)(backendcfg)
        storeclass = str(cfg.Get('StoreClass'))
        storecfg = cfg.Get('StoreCfg')
        store = import_class_from_string(storeclass)(storecfg)
        self.element = Element(self.backend, store)

    def get(self, inpayload):
        request = dict([
            (str(node.Get('requestID')), [
                (str(n.Get('property')), str(n.Get('value')))
                    for n in get_child_nodes(node.Get('getRequestDataList'))
            ]) for node in get_child_nodes(inpayload.Child().Child())])
        response = self.element.get(request)
        print response
        tree = XMLTree(from_tree =
            ('se:getResponseList', [
                ('se:getResponseElement', [
                    ('se:requestID', requestID),
                    ('se:getResponseDataList', [
                        ('se:getResponseDataElement', [
                            ('se:property', property),
                            ('se:value', value)
                        ]) for property, value in getResponseData
                    ])
                ]) for requestID, getResponseData in response.items()
            ])
        )
        out = arc.PayloadSOAP(self.se_ns)
        response_node = out.NewChild('getResponse')
        tree.add_to_node(response_node)
        return out

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
                raise Exception, 'wrong namespace (%s)' % request_name
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
    request_names = ['get']
