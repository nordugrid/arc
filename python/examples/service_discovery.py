#! /usr/bin/env python
import arc
import sys
import os

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

def example():
    # Creating a UserConfig object with the user's proxy
    # and the path of the trusted CA certificates
    uc = arc.UserConfig()
    uc.ProxyPath("/tmp/x509up_u%s" % os.getuid())
    uc.CACertificatesDirectory("/etc/grid-security/certificates")

    # Query two registries (index servers) for Computing Services
    registries = [
        # for the index1, we specify that it is an EGIIS service
        arc.Endpoint("index1.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid", arc.Endpoint.REGISTRY, "org.nordugrid.ldapegiis"),
        # for the arc-emi.grid.upjs.sk, we don't specify the type (the InterfaceName)
        # we let the system to try all possibilities
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
        # for piff, we specify that we want to query the LDAP GLUE2 tree
        arc.Endpoint("piff.hep.lu.se", arc.Endpoint.COMPUTINGINFO, "org.nordugrid.ldapglue2"),
        # for pgs03, we don't specify the interface, we let the system try all possibilities
        arc.Endpoint("pgs03.grid.upjs.sk", arc.Endpoint.COMPUTINGINFO)
    ]

    retriever2 = retrieve(uc, computing_elements)

    # Get all the ExecutionTargets on these ComputingServices
    targets2 = retriever2.GetExecutionTargets()

    print "The discovered ExecutionTargets:"
    for target in targets2:
        print target
    
    
    # Query both registries and computing elements at the same time:
    endpoints = [
        arc.Endpoint("arc-emi.grid.upjs.sk/O=Grid/Mds-Vo-Name=ARC-EMI", arc.Endpoint.REGISTRY),
        arc.Endpoint("piff.hep.lu.se", arc.Endpoint.COMPUTINGINFO, "org.nordugrid.ldapglue2")
    ]

    retriever3 = retrieve(uc, endpoints)

    print "Discovered ComputingServices:", ", ".join([service.Name for service in retriever3])


# wait for all the background threads to finish before we destroy the objects they may use
import atexit
@atexit.register
def wait_exit():
    arc.ThreadInitializer().waitExit()

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stderr))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

# run the example
example()
