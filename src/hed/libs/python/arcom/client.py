import arc
import thread, threading
from arcom.xmltree import XMLTree
import random

from arcom.logger import get_logger
log = get_logger('arcom.client')


class Client:
    """ Base Client class for sending SOAP messages to services """

    NS_class = arc.NS

    def __init__(self, url, ns, print_xml = False, xmlnode_class = arc.XMLNode, ssl_config = {}):
        """ The constructor of the Client class.
        
        Client(url, ns, print_xml = false)
        
        url is the URL of the service, it could be a list of multiple URLs
        ns contains the namespaces we want to use with each message
        print_xml is for debugging, prints all the SOAP messages to the screen
        """
        self.ns = ns
        if type(url) == list:
            urls = url
        else:
            urls = [url]
        self.urls = [arc.URL(url) for url in urls]
        self.https = 'https' in [url.Protocol() for url in self.urls]
        self.print_xml = print_xml
        self.xmlnode_class = xmlnode_class
        self.ssl_config = {}
        self.cfg = arc.MCCConfig()
        self.get_trusted_dns_method = ssl_config.get('get_trusted_dns_method', None)
        self.connection_cache = {}
        # semaphores to limit number of concurent writes
        self.semapool = threading.BoundedSemaphore(128)
        if self.https:
            self.ssl_config = ssl_config
            if ssl_config.has_key('proxy_file'):
                self.cfg.AddProxy(self.ssl_config['proxy_file'])
            else:
                try:
                    self.cfg.AddCertificate(self.ssl_config['cert_file'])
                    self.cfg.AddPrivateKey(self.ssl_config['key_file'])
                except:
                    raise Exception, 'no key file or cert file found!'
            if ssl_config.has_key('ca_file'):
                self.cfg.AddCAFile(self.ssl_config['ca_file'])
            elif ssl_config.has_key('ca_dir'):
                self.cfg.AddCADir(self.ssl_config['ca_dir'])
            else:
                raise Exception, 'no CA file or CA dir found!'
                
    def reset(self):
        tid = thread.get_ident()
        self.connection_cache[tid] = None

    def call(self, tree, return_tree_only = False):
        """ Create a SOAP message from an XMLTree and send it to the service.
        
        call(tree, return_tree_only = False)
        
        tree is an XMLTree object containing the content of the request
        return_tree_only indicates that we only need to put the response into an XMLTree
        """
        # create a new PayloadSOAP object with the given namespace
        out = arc.PayloadSOAP(self.ns)
        # add the content of the XMLTree to the XMLNode of the SOAP object
        tree.add_to_node(out)
        if self.print_xml:
            msg = out.GetXML()
            print 'Request:'
            print XMLTree(out).pretty_xml(indent = '    ', prefix = '        #   ')
            print
        # call the service and get back the response, and the HTTP status
        resp = self.call_raw(out)
        if self.print_xml:
            print 'Response:'
            try:
                print XMLTree(from_string = resp).pretty_xml(indent = '    ', prefix = '        #   ')
            except:
                print resp
            print
        if return_tree_only:
            # wrap the response into an XMLTree and return only the tree
            return XMLTree(from_string = resp, forget_namespace = True).get_trees('///')[0]
        else:
            return resp

    def _fawlty(self, message, status):
        if not status.isOk():
            return 'ERROR: %s' % status
        if message.IsFault():
            return 'ERROR: %s' % message.Fault().Reason()
        return False

    def call_raw(self, outpayload):
        """ Send a POST request with the SOAP XML message.
        
        call_raw(outpayload)
        
        outpayload is an XMLNode with the SOAP message
        """
        tid = thread.get_ident()
        s = self.connection_cache.get(tid, None)
        if s:
            try:
                resp, status = s.process(outpayload)
                if not self._fawlty(resp, status):
                    return resp.GetXML()
            except:
                pass
            self.connection_cache[tid] = None
        if len(self.urls) == 0:
            log.msg(arc.WARNING, 'No URLs to connect to (in %s)' % str(self.__class__.__name__))
            raise Exception, 'No URLs to connect'
        random.shuffle(self.urls)
        #print "available URLs", [url.fullstr() for url in self.urls]
        for url in self.urls:
            #print "trying URL", url.fullstr()
            try:
                s = arc.ClientSOAP(self.cfg, url)
                if self.get_trusted_dns_method:
                    dnlist = self.get_trusted_dns_method()
                    if dnlist:
                        dnlistconf = arc.DNListHandlerConfig(dnlist, 'outgoing')
                        # _s points to the superclass, but not the object, so it needs the object as first argument
                        s._s._s.AddSecHandler(s, dnlistconf, arc.TLSSec)
                resp, status = s.process(outpayload)
                fawlty = self._fawlty(resp, status)
                if fawlty:
                    raise Exception, fawlty
                resp = resp.GetXML()
                self.connection_cache[tid] = s
                return resp
            except:
                log.msg(arc.WARNING, "ERROR connecting to", url.fullstr())
                pass
        log.msg(arc.ERROR, "ERROR connecting to all of these:", ', '.join([url.fullstr() for url in self.urls]))
        raise
