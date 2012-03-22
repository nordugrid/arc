#! /usr/bin/env python
import arc
import sys
import os

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stdout))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

def example():
    # Creating a UserConfig object with the user's proxy
    # and the path of the trusted CA certificates
    uc = arc.UserConfig()
    uc.ProxyPath("/tmp/proxy")
    uc.CACertificatesDirectory("/tmp/certificates")

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
    print retriever.getStatusOfEndpoint(endpoint).str()

    print "Number of jobs found:", len(jobs)
    for job in jobs:
        job.SaveToStream(arc.CPyOstream(sys.stdout), True)

# run the example and catch all Exceptions in order to make sure we can call _exit() at the end
try:
    example()
except:
    import traceback
    traceback.print_exc()
    os._exit(1)

# _exit() is needed to avoid segmentation faults by still running background threads
os._exit(0)
