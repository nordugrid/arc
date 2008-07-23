import arc
import traceback
import inspect
import time
import sys

log_levels = ['VERBOSE', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'FATAL']

def get_log_system(back_level = 2):
    fr = inspect.currentframe()
    for i in range(back_level):
        fr = fr.f_back
    s = fr.f_code.co_filename
    try:
        module, package = s.split('/')[-2:]
        package = package.split('.')[0]
        s = '%s.%s' % (module, package)
    except:
        traceback.print_exc()
        pass
    return '%s:%s' % (s, fr.f_lineno)

class Service:
    
    def log(self, *args):
        """docstring for log"""
        args = list(args)
        if not args:
            severity = 'ERROR'
            args = ['Python exception:\n', traceback.format_exc()]
        elif args[0] in log_levels:
            severity = args.pop(0)
        else:
            severity = 'DEBUG'
        system = get_log_system()
        try:
            system = '%s (%s)' % (self.service_name, system)
        except:
            pass
        t = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(time.time()))
        msg = '[%s] [%s] [%s] ' % (t, system, severity) + ' '.join([str(arg) for arg in args])
        try:
            if log_levels.index(self.log_level) > log_levels.index(severity):
                return msg
        except:
            pass
        print msg
        sys.stdout.flush()
        return msg
    
    def __init__(self, service_name, request_names, namespace_prefix, namespace_uri, cfg = None):
        if cfg:
            self.log_level = str(cfg.Get('LogLevel'))
        else:
            self.log_level = None
        if not self.log_level:
            self.log_level = 'ERROR'
        self.service_name = service_name
        self.log('DEBUG', service_name, "constructor called")
        self.request_names = request_names
        self.namespace_prefix = namespace_prefix
        self.namespace_uri = namespace_uri
        self.ns = arc.NS({namespace_prefix : namespace_uri})
        
    
    def newSOAPPayload(self):
        return arc.PayloadSOAP(self.ns)
    
    def _call_request(self, request_name, inmsg):
        inpayload = inmsg.Payload()
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
                    raise Exception, 'wrong namespace (%s)' % request_name
            # get the name of the request without the namespace prefix
            request_name = request_node.Name()
            if request_name not in self.request_names:
                # if the name of the request is not in the list of supported request names
                raise Exception, 'wrong request (%s)' % request_name
            self.log('DEBUG','%s.%s called' % (self.service_name, request_name))
            # if the request name is in the supported names,
            # then this class should have a method with this name
            # the 'getattr' method returns this method
            # which then we could call with the incoming payload
            # and which will return the response payload
            self.log('VERBOSE', inpayload.GetXML())
            outpayload = self._call_request(request_name, inmsg)
            # sets the payload of the outgoing message
            outmsg.Payload(outpayload)
            # return with the STATUS_OK status
            return arc.MCC_Status(arc.STATUS_OK)
        except:
            # if there is any exception, print it
            msg = self.log()
            # TODO: need proper fault message
            outpayload = arc.PayloadSOAP(self.ns)
            outpayload.NewChild(self.namespace_prefix + ':Fault').Set(msg)
            outmsg.Payload(outpayload)
            # TODO: return with the status GENERIC_ERROR
            return arc.MCC_Status(arc.STATUS_OK)

