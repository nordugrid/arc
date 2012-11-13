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
    
    # Create a new job object with a given JobID
    job = arc.Job()
    job.JobID = arc.URL("https://piff.hep.lu.se:443/arex/1QuMDmRwvUfn5h5iWqkutBwoABFKDmABFKDmIpHKDmXBFKDmIuAean")
    job.Flavour = "ARC1"
    job.JobManagementURL = arc.URL("https://piff.hep.lu.se:443/arex")
    job.JobStatusURL = arc.URL("https://piff.hep.lu.se:443/arex")
    
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
    
# wait for all the background threads to finish before we destroy the objects they may use
import atexit
@atexit.register
def wait_exit():
    arc.ThreadInitializer().waitExit()

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stderr))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

# run the example
example()
