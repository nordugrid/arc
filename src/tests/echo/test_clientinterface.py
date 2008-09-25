#! /usr/bin/env python
import arc

import sys
root_logger = arc.Logger_getRootLogger()
root_logger.addDestination(arc.LogStream(sys.stdout))
logger = arc.Logger(root_logger, "Test")

logger.msg(arc.INFO, "Creating a soap client");

cfg = arc.MCCConfig()
cfg.AddPrivateKey('key.pem')
cfg.AddCertificate('cert.pem')
cfg.AddCAFile('ca.pem')

s = arc.ClientSOAP(cfg, 'localhost', 60000, True, '/Echo')

logger.msg(arc.INFO, "Creating and sending request")
ns = arc.NS({'echo':'urn:echo'})
outpayload = arc.PayloadSOAP(ns)
outpayload.NewChild('echo:echo').NewChild('echo:say').Set('HELLO')

resp, status = s.process(outpayload)

if not status:
    logger.msg(arc.ERROR, "SOAP invocation failed")
elif not resp:
    logger.msg(arc.ERROR, "There was no SOAP response")
else:
    print "XML:", resp.GetXML()
    print "Response:", str(resp.Get('echoResponse').Get('hear'))
