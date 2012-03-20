#! /usr/bin/env python
import arc
import sys

def retrieve(uc, endpoints):
    # The ComputingServiceRetriever needs the UserConfig to know which credentials
    # to use in case of HTTPS connections
    retriever = arc.ComputingServiceRetriever(uc, endpoints)
    # the constructor of the ComputingServiceRetriever returns immediately
    print
    print "ComputingServiceRetriever created with the following endpoints:"
    for endpoint in endpoints:
        print "-", endpoint.str()
    # here we want to wait until all the results arrive
    print "Waiting for the results..."
    retriever.wait()
    return retriever

# Creating a UserConfig object with the user's proxy
# and the path of the trusted CA certificates
uc = arc.UserConfig()
uc.ProxyPath("~/.cert/proxy")
uc.CACertificatesDirectory("~/.cert/certificates")

# Query two registries (index servers) for Computing Services
registries = [
    arc.Endpoint("index1.nordugrid.org", arc.Endpoint.REGISTRY),
    arc.Endpoint("arc-emi.grid.upjs.sk/O=Grid/Mds-Vo-Name=ARC-EMI", arc.Endpoint.REGISTRY)
]

retriever = retrieve(uc, registries)

# The retriever acts as a list containing all the discovered ComputingServices:
print "Discovered ComputingServices:", ", ".join([service.Name for service in retriever])

# Get all the ExecutionTargets on these ComputingServices
targets = retriever.GetExecutionTargets()
print "Number of ExecutionTargets on these ComputingServices:", len(targets)

# Query the local infosys (COMPUTINGINFO) of computing elements
computing_elements = [
    arc.Endpoint("piff.hep.lu.se", arc.Endpoint.COMPUTINGINFO, "org.nordugrid.ldapglue2"),
    arc.Endpoint("pgs03.grid.upjs.sk", arc.Endpoint.COMPUTINGINFO)
]

retriever2 = retrieve(uc, computing_elements)

# Get all the ExecutionTargets on these ComputingServices
targets2 = retriever2.GetExecutionTargets()

print "The discovered ExecutionTargets:"
for target in targets2:
    target.SaveToStream(arc.CPyOstream(sys.stdout), False)
    
    
# Query both registries and computing elements at the same time:
endpoints = [
    arc.Endpoint("arc-emi.grid.upjs.sk/O=Grid/Mds-Vo-Name=ARC-EMI", arc.Endpoint.REGISTRY),
    arc.Endpoint("piff.hep.lu.se", arc.Endpoint.COMPUTINGINFO, "org.nordugrid.ldapglue2")
]

retriever3 = retrieve(uc, endpoints)

print "Discovered ComputingServices:", ", ".join([service.Name for service in retriever3])

