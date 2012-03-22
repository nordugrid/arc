#! /usr/bin/env python
import arc
import sys
import os

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stderr))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

def example():
    # Creating a UserConfig object with the user's proxy
    # and the path of the trusted CA certificates
    uc = arc.UserConfig()
    uc.ProxyPath("/tmp/proxy")
    uc.CACertificatesDirectory("/tmp/certificates")
    
    # Create a new job object with a given JobID
    job = arc.Job()
    job.JobID = arc.URL("https://piff.hep.lu.se:443/arex/1QuMDmRwvUfn5h5iWqkutBwoABFKDmABFKDmIpHKDmXBFKDmIuAean")
    job.Flavour = "ARC1"
    job.Cluster = arc.URL("https://piff.hep.lu.se:443/arex")
    
    print "Job object before update:"
    job.SaveToStream(arc.CPyOstream(sys.stdout), True)
    
    job_supervisor = arc.JobSupervisor(uc, [job])

    # Update the states of jobs within this JobSupervisor
    job_supervisor.Update()
    
    # Get our updated job from the JobSupervisor
    jobs = job_supervisor.GetAllJobs()
    job = jobs[0]
    
    print "Job object after update:"
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
