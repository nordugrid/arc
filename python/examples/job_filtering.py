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
    uc.ProxyPath("/tmp/x509up_u%s" % os.getuid())
    uc.CACertificatesDirectory("/etc/grid-security/certificates")

    # Retrieve all the jobs from this computing element
    endpoint = arc.Endpoint("piff.hep.lu.se:443/arex", arc.Endpoint.COMPUTINGINFO)
    print "Querying %s for jobs..." % endpoint.str()
    jobs = arc.JobContainer()
    retriever = arc.JobListRetriever(uc)
    retriever.addConsumer(jobs)
    retriever.addEndpoint(endpoint)
    retriever.wait()
    
    print "%s jobs found" % len(jobs)
        
    # Create a JobSupervisor to handle all the jobs
    job_supervisor = arc.JobSupervisor(uc, jobs)
    
    print "Getting job states..."
    # Update the states of the jobs
    job_supervisor.Update()
    
    # Get the updated jobs
    jobs = job_supervisor.GetAllJobs()
    
    print "The jobs have the following states:", ", ".join([job.State.GetGeneralState() for job in jobs])
    
    # Select failed jobs
    job_supervisor.SelectByStatus(["Failed"])
    failed_jobs = job_supervisor.GetSelectedJobs()
    
    print "The failed jobs:"
    for job in failed_jobs:
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
