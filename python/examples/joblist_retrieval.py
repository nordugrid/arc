#! /usr/bin/env python
import arc
import sys
import os

def example():
    # Creating a UserConfig object with the user's proxy
    # and the path of the trusted CA certificates
    uc = arc.UserConfig()
    uc.ProxyPath("/tmp/x509up_u%s" % os.getuid())
    uc.CACertificatesDirectory("/etc/grid-security/certificates")

    # Creating an endpoint for a Computing Element
    endpoint = arc.Endpoint("piff.hep.lu.se:443/arex", arc.Endpoint.COMPUTINGINFO)

    # Creating a container which will store the retrieved jobs
    jobs = arc.JobContainer()

    # Create a job list retriever
    retriever = arc.JobListRetriever(uc)
    # Add our container as the consumer of this retriever, so it will get the results
    retriever.addConsumer(jobs)

    # Add our endpoint to the retriever, which starts querying it
    retriever.addEndpoint(endpoint)

    # Wait until it finishes
    retriever.wait()

    # Get the status of the retrieval
    sys.stdout.write("%s\n"%retriever.getStatusOfEndpoint(endpoint).str())

    sys.stdout.write("Number of jobs found: %d\n"%len(jobs))
    for job in jobs:
        job.SaveToStream(arc.CPyOstream(sys.stdout), True)

# wait for all the background threads to finish before we destroy the objects they may use
import atexit
@atexit.register
def wait_exit():
    arc.ThreadInitializer().waitExit()

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stderr))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

# run the example
example()
