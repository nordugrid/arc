import arc
import time
from storage.common import parse_ssl_config
from storage.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'EchoService.py'))

class EchoService:

    def __init__(self, cfg):
        log.msg("EchoService (python) constructor called")
        # get the response-prefix from the config XML
        self.prefix = str(cfg.Get('prefix'))
        # get the response-suffix from the config XML
        self.suffix = str(cfg.Get('suffix'))
        log.msg("EchoService (python) has prefix '%s' and suffix '%s'" % (self.prefix, self.suffix))
        self.ssl_config = parse_ssl_config(cfg)
        

    def process(self, inmsg, outmsg):
        log.msg("EchoService (python) 'Process' called")
        # time.sleep(10)
        # get the payload from the message
        inpayload = inmsg.Payload()
        a = inmsg.Auth()
        x = a.Export(arc.SecAttr.ARCAuth)
        print x.GetXML()
        attributes = inmsg.Attributes()
        log.msg(arc.INFO, "EchoService (python) got:", inpayload.GetXML())
        # the first child of the payload should be the name of the request
        request_node = inpayload.Child()
        # we want to use members of the 'urn:echo' namespace
        # so we need to know which namespace prefix was assigned to it
        # we get the namespace prefix
        echo_ns = request_node.NamespacePrefix('urn:echo')
        # get the qualified name of the request
        request_name = request_node.FullName()
        log.msg("EchoService (python) request_name:",  request_name)
        if not request_name.startswith(echo_ns + ':'):
            raise Exception, 'wrong namespace. expected: %s' % ('urn:echo')
        # get the name of the request without the namespace prefix
        # this is the name of the Body node's first child
        request_name = request_node.Name()
        # create an answer payload
        ns = arc.NS({'echo': 'urn:echo'})
        outpayload = arc.PayloadSOAP(ns)
        # here we defined that 'echo' prefix will be the namespace prefix of 'urn:echo'
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
                if self.ssl_config.has_key('ca_file'):
                    cfg.AddCAFile(self.ssl_config.get('ca_file', None))
                else:
                    cfg.AddCADir(self.ssl_config.get('ca_dir', None))
                ssl = True
            if ssl: 	    
                url = arc.URL('https://localhost:60000/Echo')
                log.msg('Calling https://localhost:60000/Echo using ClientSOAP')
            else: 	    
                url = arc.URL('http://localhost:60000/Echo')
                log.msg('Calling http://localhost:60000/Echo using ClientSOAP')
            # creating the ClientSOAP object
            s = arc.ClientSOAP(cfg, url)
            new_payload = arc.PayloadSOAP(ns)
            # creating the message
            new_payload.NewChild('echo:echo').NewChild('echo:say').Set(hear)
            log.msg('new_payload', new_payload.GetXML())
            # sending the message
            resp, status = s.process(new_payload)
            # get the response
            hear = str(resp.Get('echoResponse').Get('hear'))
        elif request_name == 'httplib':
            # if the name of the request is 'httplib'
            # we create a new echo message which we send to http://localhost:60000/echo using python's built-in http client
            import httplib
            log.msg('Calling http://localhost:60000/Echo using httplib')
            # create the connection
            h = httplib.HTTPConnection('localhost', 60000)
            new_payload = arc.PayloadSOAP(ns)
            # create the message
            new_payload.NewChild('echo:echo').NewChild('echo:say').Set(hear)
            log.msg('new_payload', new_payload.GetXML())
            # send the message
            h.request('POST', '/Echo', new_payload.GetXML())
            r = h.getresponse()
            response = r.read()
            log.msg(response)
            resp = arc.XMLNode(response)
            # get the response
            hear = str(resp.Child().Get('echoResponse').Get('hear'))
        elif request_name == 'wait':
            log.msg('Start waiting 10 sec...')
            time.sleep(10)
            log.msg('Waiting ends.')
        # we create a node at '/echo:echoResponse/echo:hear' and put the string in it
        outpayload.NewChild('echo:echoResponse').NewChild('echo:hear').Set(hear)
        outmsg.Payload(outpayload)
        log.msg("outpayload", outpayload.GetXML())
        # return with STATUS_OK
        return arc.MCC_Status(arc.STATUS_OK)

# you can easily test this with this shellscript:
"""
MESSAGE='<?xml version="1.0"?><soap-env:Envelope xmlns:soap-enc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:star="urn:echo"><soap-env:Body><star:echo><star:say>HELLO</star:say></star:echo></soap-env:Body></soap-env:Envelope>'
echo Request:
echo $MESSAGE
echo
echo Response:
curl -d "$MESSAGE" http://localhost:60000/Echo
echo
"""
#
