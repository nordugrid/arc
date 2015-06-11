#! /usr/bin/env python
import arc
import sys

def example():
    uc = arc.UserConfig()

    # Create a JobSupervisor to handle all the jobs
    job_supervisor = arc.JobSupervisor(uc)

    # Retrieve all the jobs from this computing element
    endpoint = arc.Endpoint("https://piff.hep.lu.se:443/arex", arc.Endpoint.JOBLIST)
    sys.stdout.write("Querying %s for jobs...\n" % endpoint.str())
    retriever = arc.JobListRetriever(uc)
    retriever.addConsumer(job_supervisor)
    retriever.addEndpoint(endpoint)
    retriever.wait()

    sys.stdout.write("%s jobs found\n" % len(job_supervisor.GetAllJobs()))

    sys.stdout.write("Getting job states...\n")
    # Update the states of the jobs
    job_supervisor.Update()

    # Print state of updated jobs
    sys.stdout.write("The jobs have the following states: %s\n"%(", ".join([job.State.GetGeneralState() for job in job_supervisor.GetAllJobs()])))

    # Select failed jobs
    job_supervisor.SelectByStatus(["Failed"])
    failed_jobs = job_supervisor.GetSelectedJobs()

    sys.stdout.write("The failed jobs:\n")
    for job in failed_jobs:
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
