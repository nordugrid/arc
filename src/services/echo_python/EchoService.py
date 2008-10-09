import arc
import time

class EchoService:

    def __init__(self, cfg):
        print "EchoService (python) constructor called"
        # get the response-prefix from the config XML
        self.prefix = str(cfg.Get('prefix'))
        # get the response-suffix from the config XML
        self.suffix = str(cfg.Get('suffix'))
        print "EchoService (python) has prefix '%s' and suffix '%s'" % (self.prefix, self.suffix)

    def process(self, inmsg, outmsg):
        print "EchoService (python) 'Process' called"
        # time.sleep(10)
        # get the payload from the message
        inpayload = inmsg.Payload()
        attributes = inmsg.Attributes()
        print attributes.getAll()
        print "EchoService (python) got:", inpayload.GetXML()
        # the first child of the payload should be the name of the request
        request_node = inpayload.Child()
        # we want to use members of the 'urn:echo' namespace
        # so we need to know which namespace prefix was assigned to it
        # we get the namespace prefix
        echo_ns = request_node.NamespacePrefix('urn:echo')
        # get the qualified name of the request
        request_name = request_node.FullName()
        print "EchoService (python) request_name:",  request_name
        if not request_name.startswith(echo_ns + ':'):
            raise Exception, 'wrong namespace. expected: %s' % ('urn:echo')
        # get the name of the request without the namespace prefix
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
            # let's call http://localhost:60000/Echo
            cfg = arc.MCCConfig()
            s = arc.ClientSOAP(cfg, 'localhost', 60000, False, '/Echo')
            new_payload = arc.PayloadSOAP(ns)
            new_payload.NewChild('echo:echo').NewChild('echo:say').Set(hear)
            print 'new_payload', new_payload.GetXML()
            resp, status = s.process(new_payload)
            hear = str(resp.Get('echoResponse').Get('hear'))
        # we create a node at '/echo:echoResponse/echo:hear' and put the string in it
        outpayload.NewChild('echo:echoResponse').NewChild('echo:hear').Set(hear)
        outmsg.Payload(outpayload)
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
