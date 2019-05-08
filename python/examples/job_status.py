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
    job.JobID = "https://piff.hep.lu.se:443/arex/w7LNDmSkEiun1ZPzno6AuCjpABFKDmABFKDmZ9LKDmUBFKDmXugZwm"
    job.IDFromEndpoint = "w7LNDmSkEiun1ZPzno6AuCjpABFKDmABFKDmZ9LKDmUBFKDmXugZwm"
    job.JobManagementURL = arc.URL("https://piff.hep.lu.se:443/arex")
    job.JobStatusURL = arc.URL("https://piff.hep.lu.se:443/arex")
    job.JobStatusInterfaceName = 'org.ogf.glue.emies.activitymanagement'
    job.JobManagementInterfaceName = 'org.ogf.glue.emies.activitymanagement'
    
    sys.stdout.write("Job object before update:\n")
    job.SaveToStream(arc.CPyOstream(sys.stdout), True)
    
    job_supervisor = arc.JobSupervisor(uc, [job])

    # Update the states of jobs within this JobSupervisor
    job_supervisor.Update()
    
    # Get our updated job from the JobSupervisor
    jobs = job_supervisor.GetAllJobs()
    if not jobs:
        sys.stdout.write("No jobs found\n")
        return

    job = jobs[0]
    
    sys.stdout.write("Job object after update:\n")
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
