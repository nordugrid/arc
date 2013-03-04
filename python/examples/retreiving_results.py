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
    job.JobID = "https://piff.hep.lu.se:443/arex/hYDLDmyxvUfn5h5iWqkutBwoABFKDmABFKDmIpHKDmYBFKDmtRy9En"
    job.Flavour = "ARC1"
    job.ServiceInformationURL = job.JobStatusURL = job.JobManagementURL = arc.URL("https://piff.hep.lu.se:443/arex")
    
    print "Get job information from the computing element..."
    # Put the job into a JobSupervisor and update its information
    job_supervisor = arc.JobSupervisor(uc, [job])
    job_supervisor.Update()
    
    print "Downloading results..."
    # Prepare a list for storing the directories for the downloaded job results (if there would be more jobs)
    downloadeddirectories = arc.StringList()
    # Start retrieving results of all the selected jobs
    #   into the "/tmp" directory (first argument)
    #   using the jobid and not the jobname as the name of the subdirectory (second argument, usejobname = False)
    #   do not overwrite existing directories with the same name (third argument: force = False)
    #   collect the downloaded directories into the variable "downloadeddirectories" (forth argument)
    success = job_supervisor.Retrieve("/tmp", False, False, downloadeddirectories)
    if not success:
        print "Downloading results failed."
    for downloadeddirectory in downloadeddirectories:
        print "Job results were downloaded to", downloadeddirectory
        print "Contents of the directory:"
        for filename in os.listdir(downloadeddirectory):
            print "  ", filename
        
# wait for all the background threads to finish before we destroy the objects they may use
import atexit
@atexit.register
def wait_exit():
    arc.ThreadInitializer().waitExit()

# arc.Logger.getRootLogger().addDestination(arc.LogStream(sys.stderr))
# arc.Logger.getRootLogger().setThreshold(arc.DEBUG)

# run the example
example()
