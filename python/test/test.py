#!/usr/bin/env python
#import sys
#sys.path=sys.path+['../', '../.libs/' ]
import arc
f = open('soap.xml').read()
e = arc.SOAPEnvelope(f)
print e.toBool()
p = arc.PayloadSOAP(e)
print p.toBool(), p.Size()
g = p.Get("echo").Get("say")
print str(g)
