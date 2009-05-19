#! /usr/bin/env python
import sys, time
import arc
from arcom.client import Client
from arcom.service import ahash_uri
from arcom.xmltree import XMLTree

url = arc.URL("http://dingding.uio.no:60000/AHash")
ns = arc.NS("ahash", ahash_uri)
cfg = arc.MCCConfig()
outpayload = arc.PayloadSOAP(ns)

n_calls = 1000

IDs = ['0']
tree = XMLTree(from_tree =
               ('ahash:get', [],
                   ('ahash:IDs', [
                       ('ahash:ID', i) for i in IDs
              ])
        ))

tree.add_to_node(outpayload)
############# re-using ClientSOAP
caching_proc_time = 0
s = arc.ClientSOAP(cfg,url)
for i in range(n_calls):
    process_start = time.time()
    s.process(outpayload)
    caching_proc_time += time.time()-process_start

print "Average time to call ClientSOAP.process %d times reusing ClientSOAP object: %g"%(n_calls,caching_proc_time/n_calls)

############# creating new ClientSOAP for every call
nocaching_proc_time = 0
for i in range(n_calls):
    s = arc.ClientSOAP(cfg,url)
    process_start = time.time()
    s.process(outpayload)
    nocaching_proc_time += time.time()-process_start

print "Average time to call ClientSOAP.process %d times creating new ClientSOAP objects every time: %g"%(n_calls,nocaching_proc_time/n_calls)

print "Difference in seconds (caching-nocaching):", (caching_proc_time-nocaching_proc_time)/n_calls
