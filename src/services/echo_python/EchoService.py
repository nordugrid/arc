import arc

class EchoService:

    def __init__(self):
        print "EchoService constructor called"

    def process(self, inmsg, outmsg):
        print "Process called"
        print type(inmsg)
        inpayload = inmsg.Payload()
        print type(inpayload)
        print dir(inpayload)
        echo_op = inpayload.Get("echo")
        print echo_op
        say = echo_op.Get("say")
        hear = '[' + say + ']'
        ns = arc.Namespace("echo")
        outpayload = arc.PayloadSOAP(ns)
        outpayload.NewChild("echo:echoResponse").NewChild("echo:hear").Set(hear)
        outmsg.Payload(outpayload)
        return arc.MCC_Status(arc.STATUS_OK)
