#! /usr/bin/env python
import arc
import sys
import os

def retrieve(uc, endpoints):
    # The ComputingServiceRetriever needs the UserConfig to know which credentials
    # to use in case of HTTPS connections
    retriever = arc.ComputingServiceRetriever(uc, endpoints)
    # the constructor of the ComputingServiceRetriever returns immediately
    sys.stdout.write('\n')
    sys.stdout.write("ComputingServiceRetriever created with the following endpoints:\n")
    for endpoint in endpoints:
        sys.stdout.write("- %s\n"%endpoint.str())
    # here we want to wait until all the results arrive
    sys.stdout.write("Waiting for the results...\n")
    retriever.wait()
    return retriever

def example():
    # Creating a UserConfig object with the user's proxy
    # and the path of the trusted CA certificates
    uc = arc.UserConfig()
    uc.ProxyPath("/tmp/x509up_u%s" % os.getuid())
    uc.CACertificatesDirectory("/etc/grid-security/certificates")

    # Query registriy (index server) for Computing Services
    registries = [
        arc.Endpoint("nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.archery")
    ]

    retriever = retrieve(uc, registries)

    # The retriever acts as a list containing all the discovered ComputingServices:
    sys.stdout.write("Discovered ComputingServices: %s\n"%(", ".join([service.Name for service in retriever])))

    # Get all the ExecutionTargets on these ComputingServices
    targets = retriever.GetExecutionTargets()
    sys.stdout.write("Number of ExecutionTargets on these ComputingServices: %d\n"%len(targets))

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

    sys.stdout.write("The discovered ExecutionTargets:\n")
    for target in targets2:
        sys.stdout.write("%s\n"%str(target))


    # Query both registries and computing elements at the same time:
    endpoints = [
        arc.Endpoint("arc-emi.grid.upjs.sk/O=Grid/Mds-Vo-Name=ARC-EMI", arc.Endpoint.REGISTRY),
        arc.Endpoint("piff.hep.lu.se", arc.Endpoint.COMPUTINGINFO, "org.nordugrid.ldapglue2")
    ]

    retriever3 = retrieve(uc, endpoints)

    sys.stdout.write("Discovered ComputingServices: %s\n"%(", ".join([service.Name for service in retriever3])))


# wait for all the background threads to finish before we destroy the objects they may use
import atexit
@atexit.register
def wait_exit():
    arc.ThreadInitializer().waitExit()

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stderr))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

# run the example
example()
