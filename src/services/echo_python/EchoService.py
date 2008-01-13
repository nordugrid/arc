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
        print "EchoService (python) got:", inpayload.GetXML()
        # we want to use members of the 'urn:echo' namespace
        # so we need to know which namespace prefix was assigned to it
        # we get the namespace prefix
        echo_ns = inpayload.NamespacePrefix('urn:echo')
        # then we can get 'prefix:echo/prefix:say'
        echo_op = inpayload.Get(echo_ns + ':echo')
        say = str(echo_op.Get(echo_ns + ':say'))
        # put it between the response-prefix and the response-suffix
        hear = self.prefix + say + self.suffix
        # create an answer payload
        outpayload = arc.PayloadSOAP(arc.NS({'echo':'urn:echo'}))
        # here we defined that 'echo' prefix will be the namespace prefix of 'urn:echo'
        # and we create a node at '/echo:echoResponse/echo:hear' and put the string in it
        outpayload.NewChild('echo:echoResponse').NewChild('echo:hear').Set(hear)
        # put the payload into the outgoing message
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
