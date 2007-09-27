import arc

class EchoService:

    def __init__(self):
        print "EchoService constructor called"

    def process(self, inmsg, outmsg):
        print "Process called"
        inpayload = inmsg.Payload()
        echo_op = inpayload.Get("echo")
        say = str(echo_op.Get("say"))
        hear = '[' + say + ']'
        ns = arc.NS()
        ns["echo"] = "urn:echo"
        outpayload = arc.PayloadSOAP(ns)
        outpayload.NewChild("echo:echoResponse").NewChild("echo:hear").Set(hear)
        outmsg.Payload(outpayload)
        return arc.MCC_Status(arc.STATUS_OK)
