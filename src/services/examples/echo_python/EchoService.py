import arc
import time
logger = arc.Logger(arc.Logger_getRootLogger(), 'EchoService.py')

wsrf_rp_ns = "http://docs.oasis-open.org/wsrf/rp-2"
echo_ns = "http://www.nordugrid.org/schemas/echo"
import threading

class EchoService(object):

    def __init__(self, cfg):
        logger.msg(arc.INFO, "EchoService (python) constructor called")
        # get the response-prefix from the config XML
        self.prefix = str(cfg.Get('prefix'))
        # get the response-suffix from the config XML
        self.suffix = str(cfg.Get('suffix'))
        logger.msg(arc.DEBUG, "EchoService (python) has prefix %(prefix)s and suffix %(suffix)s" % {'prefix': self.prefix, 'suffix': self.suffix})
        self.ssl_config = self.parse_ssl_config(cfg)
        thread_test = str(cfg.Get('ThreadTest'))
        if thread_test:
            threading.Thread(target = self.infinite, args=[thread_test]).start()
        
    def __del__(self):
        logger.msg(arc.INFO, "EchoService (python) destructor called")

    def parse_ssl_config(self, cfg):
        try:
            client_ssl_node = cfg.Get('ClientSSLConfig')
            fromFile = str(client_ssl_node.Attribute('FromFile'))
            if fromFile:
                try:
                    xml_string = file(fromFile).read()
                    client_ssl_node = arc.XMLNode(xml_string)
                except:
                    log.msg()
                    pass
            if client_ssl_node.Size() == 0:
                return {}
            ssl_config = {}
            ssl_config['key_file'] = str(client_ssl_node.Get('KeyPath'))
            ssl_config['cert_file'] = str(client_ssl_node.Get('CertificatePath'))
            ca_file = str(client_ssl_node.Get('CACertificatePath'))
            if ca_file:
                ssl_config['ca_file'] = ca_file
            else:
                ssl_config['ca_dir'] = str(client_ssl_node.Get('CACertificatesDir'))
            return ssl_config
        except:
            import traceback
            logger.msg(arc.ERROR, traceback.format_exc())
            return {}
    
    def infinite(self, url):
        logger.msg(arc.INFO, "EchoService (python) thread test starting")
        i = 0
        while True:
            try:
                i += 1
                cfg = arc.MCCConfig()
                s = arc.ClientSOAP(cfg, arc.URL(url))
                ns = arc.NS('echo', echo_ns)
                outpayload = arc.PayloadSOAP(ns)
                outpayload.NewChild('echo:echo').NewChild('echo:say').Set('hi!')
                resp, status = s.process(outpayload)
                logger.msg(arc.INFO, "EchoService (python) thread test, iteration %(iteration)s %(status)s" % {'iteration': i, 'status': status})
                time.sleep(3)
            except Exception as e:
                import traceback
                logger.msg(arc.DEBUG, traceback.format_exc())

    def RegistrationCollector(self, doc):
        regentry = arc.XMLNode('<RegEntry />')
        SrcAdv = regentry.NewChild('SrcAdv')
        SrcAdv.NewChild('Type').Set('org.nordugrid.tests.echo_python')

        #Place the document into the doc attribute
        doc.Replace(regentry)
        return True

    def GetLocalInformation(self):
        ns = arc.NS({'':'http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01'})
        info = arc.XMLNode(ns,'Domains')
        service_node = info.NewChild('AdminDomain').NewChild('Services').NewChild('Service')
        service_node.NewChild('Type').Set('org.nordugrid.tests.echo_python')
        endpoint_node = service_node.NewChild('Endpoint')
        endpoint_node.NewChild('HealthState').Set('ok')
        endpoint_node.NewChild('ServingState').Set('production')    
        return info

    def process(self, inmsg, outmsg):
        logger.msg(arc.DEBUG, "EchoService (python) 'Process' called")
        # time.sleep(10)
        # get the payload from the message
        inpayload = inmsg.Payload()
        logger.msg(arc.VERBOSE, 'inmsg.Auth().Export(arc.SecAttr.ARCAuth) = %s' % inmsg.Auth().Export(arc.SecAttr.ARCAuth).GetXML())
        logger.msg(arc.VERBOSE, 'inmsg.Attributes().getAll() = %s ' % inmsg.Attributes().getAll())
        logger.msg(arc.INFO, "EchoService (python) got: %s " % inpayload.GetXML())
        # the first child of the payload should be the name of the request
        request_node = inpayload.Child()
        # get the namespace
        request_namespace = request_node.Namespace()
        logger.msg(arc.DEBUG, "EchoService (python) request_namespace: %s" % request_namespace)
        if request_namespace != echo_ns:
            if request_namespace == wsrf_rp_ns:
                outpayload = arc.PayloadSOAP(arc.NS({'wsrf-rp':wsrf_rp_ns}))
                outpayload.NewChild('wsrf-rp:GetResourcePropertyDocumentResponse').NewChild(self.GetLocalInformation())
                outmsg.Payload(outpayload)
                logger.msg(arc.DEBUG, "outpayload %s" % outpayload.GetXML())
                return arc.MCC_Status(arc.STATUS_OK)
            raise Exception('wrong namespace. expected: %s' % echo_ns)
        # get the name of the request without the namespace prefix
        # this is the name of the Body node's first child
        request_name = request_node.Name()
        # create an answer payload
        ns = arc.NS({'echo': echo_ns})
        outpayload = arc.PayloadSOAP(ns)
        # here we defined that 'echo' prefix will be the namespace prefix of 'http://www.nordugrid.org/schemas/echo'
        # get the message
        say = str(request_node.Get('say'))
        # put it between the response-prefix and the response-suffix
        hear = self.prefix + say + self.suffix
        if request_name == 'double':
            # if the name of the request is 'double'
            # we create a new echo message which we send to http://localhost:60000/Echo using the ClientSOAP object
            cfg = arc.MCCConfig()
            ssl = False
            if self.ssl_config:
                cfg.AddCertificate(self.ssl_config.get('cert_file', None))
                cfg.AddPrivateKey(self.ssl_config.get('key_file', None))
                if 'ca_file' in self.ssl_config:
                    cfg.AddCAFile(self.ssl_config.get('ca_file', None))
                else:
                    cfg.AddCADir(self.ssl_config.get('ca_dir', None))
                ssl = True
            if ssl:
                url = arc.URL('https://localhost:60000/Echo')
                logger.msg(arc.DEBUG, 'Calling https://localhost:60000/Echo using ClientSOAP')
            else:
                url = arc.URL('http://localhost:60000/Echo')
                logger.msg(arc.DEBUG, 'Calling http://localhost:60000/Echo using ClientSOAP')
            # creating the ClientSOAP object
            s = arc.ClientSOAP(cfg, url)
            new_payload = arc.PayloadSOAP(ns)
            # creating the message
            new_payload.NewChild('echo:echo').NewChild('echo:say').Set(hear)
            logger.msg(arc.DEBUG, 'new_payload %s' % new_payload.GetXML())
            # sending the message
            resp, status = s.process(new_payload)
            # get the response
            hear = str(resp.Get('echoResponse').Get('hear'))
        elif request_name == 'httplib':
            # if the name of the request is 'httplib'
            # we create a new echo message which we send to http://localhost:60000/echo using python's built-in http client
            try:
                import http.client as httplib
            except ImportError:
                import httplib
            logger.msg(arc.DEBUG, 'Calling http://localhost:60000/Echo using httplib')
            # create the connection
            h = httplib.HTTPConnection('localhost', 60000)
            new_payload = arc.PayloadSOAP(ns)
            # create the message
            new_payload.NewChild('echo:echo').NewChild('echo:say').Set(hear)
            logger.msg(arc.DEBUG, 'new_payload %s' % new_payload.GetXML())
            # send the message
            h.request('POST', '/Echo', new_payload.GetXML())
            r = h.getresponse()
            response = r.read()
            logger.msg(arc.DEBUG, response)
            resp = arc.XMLNode(response)
            # get the response
            hear = str(resp.Child().Get('echoResponse').Get('hear'))
        elif request_name == 'wait':
            logger.msg(arc.DEBUG, 'Start waiting 10 sec...')
            time.sleep(10)
            logger.msg(arc.DEBUG, 'Waiting ends.')
        # we create a node at '/echo:echoResponse/echo:hear' and put the string in it
        outpayload.NewChild('echo:echoResponse').NewChild('echo:hear').Set(hear)
        outmsg.Payload(outpayload)
        logger.msg(arc.DEBUG, "outpayload %s" % outpayload.GetXML())
        # return with STATUS_OK
        return arc.MCC_Status(arc.STATUS_OK)

# you can easily test this with this shellscript:
"""
MESSAGE='<?xml version="1.0"?><soap-env:Envelope xmlns:soap-enc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:star="http://www.nordugrid.org/schemas/echo"><soap-env:Body><star:echo><star:say>HELLO</star:say></star:echo></soap-env:Body></soap-env:Envelope>'
echo Request:
echo $MESSAGE
echo
echo Response:
curl -d "$MESSAGE" http://localhost:60000/Echo
echo
"""
#
