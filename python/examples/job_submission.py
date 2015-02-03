#! /usr/bin/env python
import arc
import sys
import os
import random

def example():
    # Creating a UserConfig object with the user's proxy
    # and the path of the trusted CA certificates
    uc = arc.UserConfig()
    uc.ProxyPath("/tmp/x509up_u%s" % os.getuid())
    uc.CACertificatesDirectory("/etc/grid-security/certificates")

    # Creating an endpoint for a Computing Element
    endpoint = arc.Endpoint("piff.hep.lu.se", arc.Endpoint.COMPUTINGINFO, "org.nordugrid.ldapglue2")

    # Get the ExecutionTargets of this ComputingElement
    retriever = arc.ComputingServiceRetriever(uc, [endpoint])
    retriever.wait()
    targets = retriever.GetExecutionTargets()    

    # Shuffle the targets to simulate a random broker
    targets = list(targets)
    random.shuffle(targets)
    
    # Create a JobDescription
    jobdesc = arc.JobDescription()
    jobdesc.Application.Executable.Path = "/bin/hostname"
    jobdesc.Application.Output = "stdout.txt"

    # create an empty job object which will contain our submitted job
    job = arc.Job()
    success = False;
    # Submit job directly to the execution targets, without a broker
    for target in targets:
        print("Trying to submit to %s (%s) ..."%(target.ComputingEndpoint.URLString, target.ComputingEndpoint.InterfaceName))
        sys.stdout.flush()
        success = target.Submit(uc, jobdesc, job)
        if success:
            print("succeeded!")
            break
        else:
            print("failed!")
    if success:
        print("Job was submitted:")
        job.SaveToStream(arc.CPyOstream(sys.stdout), False)
    else:
        print("Job submission failed")
    
# wait for all the background threads to finish before we destroy the objects they may use
import atexit
@atexit.register
def wait_exit():
    arc.ThreadInitializer().waitExit()

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stderr))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

# run the example
example()
