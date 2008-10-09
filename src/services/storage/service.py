import arc
import inspect
import time
import sys
from storage.common import AuthRequest

from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.Service'))

class Service:
    
    def __init__(self, service_name, request_names, namespace_prefix, namespace_uri, cfg = None):
        #if cfg:
        #    log_level = str(cfg.Get('LogLevel'))
        #else:
        #    log_level = None
        self.service_name = service_name
        log.msg(arc.DEBUG, service_name, "constructor called")
        self.request_names = request_names
        self.namespace_prefix = namespace_prefix
        self.namespace_uri = namespace_uri
        self.ns = arc.NS(namespace_prefix, namespace_uri)
        
    
    def newSOAPPayload(self):
        return arc.PayloadSOAP(self.ns)
    
    def _call_request(self, request_name, inmsg):
        inpayload = inmsg.Payload()
        attributes = inmsg.Attributes().getAll()
        auth = AuthRequest()
        identity = attributes.get('TLS:IDENTITYDN', '')
        if identity:
            auth['identity'] = identity
        inpayload.auth = auth
        return getattr(self,request_name)(inpayload)
    
    def process(self, inmsg, outmsg):
        """ Method to process incoming message and create outgoing one. """
        # gets the payload from the incoming message
        inpayload = inmsg.Payload()
        try:
            # gets the namespace prefix of the service's namespace in its incoming payload
            prefix = inpayload.NamespacePrefix(self.namespace_uri)
            # the first child of the payload should be the name of the request
            request_node = inpayload.Child()
            # get the qualified name of the request
            request_name = request_node.FullName()
            #print request_name
            if not request_name.startswith(prefix + ':'):
                # if the request is not in the service's namespace
                prefix = request_node.NamespacePrefix(self.namespace_uri)
                if not request_name.startswith(prefix + ':'):
                    raise Exception, 'wrong namespace. expected: %s' % (self.namespace_uri)
            # get the name of the request without the namespace prefix
            request_name = request_node.Name()
            if request_name not in self.request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            log.msg(arc.DEBUG,'%s.%s called' % (self.service_name, request_name))
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            log.msg(arc.VERBOSE, inpayload.GetXML())
            outpayload = self._call_request(request_name, inmsg)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            msg = log.msg()
            # TODO: need proper fault message
            outpayload = arc.PayloadSOAP(self.ns)
            outpayload.NewChild(self.namespace_prefix + ':Fault').Set(msg)
            outmsg.Payload(outpayload)
            # TODO: return with the status GENERIC_ERROR
            return arc.MCC_Status(arc.STATUS_OK)

