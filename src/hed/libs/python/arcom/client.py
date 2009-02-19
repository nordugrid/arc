import arc
from arcom.xmltree import XMLTree

class Client:
    """ Base Client class for sending SOAP messages to services """

    NS_class = arc.NS

    def __init__(self, url, ns, print_xml = False, xmlnode_class = arc.XMLNode, ssl_config = {}):
        """ The constructor of the Client class.
        
        Client(url, ns, print_xml = false)
        
        url is the URL of the service
        ns contains the namespaces we want to use with each message
        print_xml is for debugging, prints all the SOAP messages to the screen
        """
        self.ns = ns
        self.url = arc.URL(url)
        self.print_xml = print_xml
        self.xmlnode_class = xmlnode_class
        self.connection = None
        self.ssl_config = {}
        self.cfg = arc.MCCConfig()
        if self.url.Protocol() == 'https':
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

    def call_raw(self, outpayload):
        """ Send a POST request with the SOAP XML message.
        
        call_raw(outpayload)
        
        outpayload is an XMLNode with the SOAP message
        """
        try:
            s = arc.ClientSOAP(self.cfg, self.url)          
            resp, status = s.process(outpayload)
            resp = resp.GetXML()
            return resp
        except:
            #TODO: print "ERROR connecting to '%s%s:%s%s'" % (self.ssl_config and 'https://' or 'http://', self.host, self.port, self.path)
            raise
