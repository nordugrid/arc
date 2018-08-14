#! /usr/bin/env python

from __future__ import print_function

import arc
import sys
root_logger = arc.Logger_getRootLogger()
root_logger.addDestination(arc.LogStream(sys.stdout))
root_logger.setThreshold(arc.ERROR)
if len(sys.argv) < 2:
    print("Usage: echo_client.py URL [message]")
    print("  echo_client gets the credentials from the default user config file")
    sys.exit(-1)
url = arc.URL(sys.argv[1])
try:
    message = sys.argv[2]
except:
    message = 'hi!'
cfg = arc.MCCConfig()
uc = arc.UserConfig('')
uc.ApplyToConfig(cfg)
s = arc.ClientSOAP(cfg, url)
outpayload = arc.PayloadSOAP(arc.NS('echo', 'http://www.nordugrid.org/schemas/echo'))
outpayload.NewChild('echo:echo').NewChild('echo:say').Set(message)
resp, status = s.process(outpayload)
print(resp.GetXML(True))
