import arc

class EchoService:

    def __init__(self, cfg):
        print "EchoService (python) constructor called"
        self.prefix = str(cfg.Get('prefix'))
        self.suffix = str(cfg.Get('suffix'))
        print "EchoService (python) has prefix '%s' and suffix '%s'" % (self.prefix, self.suffix)

    def process(self, inmsg, outmsg):
        print "EchoService (python) 'Process' called"
        inpayload = inmsg.Payload()
        print "EchoService (python) got:", inpayload.GetXML()
        echo_op = inpayload.Get("echo")
        say = str(echo_op.Get("say"))
        hear = self.prefix + say + self.suffix
        ns = arc.NS()
        ns['echo'] = 'urn:echo'
        outpayload = arc.PayloadSOAP(ns)
        outpayload.NewChild("echo:echoResponse").NewChild("echo:hear").Set(hear)
        outmsg.Payload(outpayload)
        return arc.MCC_Status(arc.STATUS_OK)
