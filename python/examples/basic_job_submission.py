import arc
import sys

# Set up logging to stderr with level VERBOSE (a lot of output will be shown)
logstdout = arc.LogStream(sys.stdout)
logstdout.setFormat(arc.ShortFormat)
arc.Logger_getRootLogger().addDestination(logstdout)
arc.Logger_getRootLogger().setThreshold(arc.VERBOSE)
logger = arc.Logger(arc.Logger_getRootLogger(), "jobsubmit")

# UserConfig contains information on credentials and default services to use.
# This form of the constructor is necessary to initialise the local job list.
usercfg = arc.UserConfig("", "")

# Two simple job descriptions which output hostname to stdout
jobdescstring = "+(&(executable=/bin/hostname)(stdout=stdout))(&(executable=/bin/hostname)(stdout=stdout))"

# Parse job description
jobdescs = arc.JobDescriptionList()
if not arc.JobDescription_Parse(jobdescstring, jobdescs):
    logger.msg(arc.ERROR, "Invalid job description")
    sys.exit(1)

# Use 'arc.JobDescription_ParseFromFile("helloworld.xrsl", jobdescs)'
# to parse job description from file.

# Use top-level NorduGrid information index to find resources
index = arc.Endpoint("nordugrid.org", arc.Endpoint.REGISTRY, "org.nordugrid.archery")
services = arc.EndpointList(1, index)

# Do the submission
jobs = arc.JobList()
submitter = arc.Submitter(usercfg)
if submitter.BrokeredSubmit(services, jobdescs, jobs) != arc.SubmissionStatus.NONE:
    logger.msg(arc.ERROR, "Failed to submit job")
    sys.exit(1)

# Write information on submitted job to local job list (~/.arc/jobs.xml)
jobList = arc.JobInformationStorageSQLite(usercfg.JobListFile())
if not jobList.Write(jobs):
    logger.msg(arc.WARNING, "Failed to write to local job list %s" % usercfg.JobListFile())

# Job submitted ok
sys.stdout.write("Job(s) submitted with job id(s):\n\t%s\n" % '\n\t'.join([str(j.JobID) for j in jobs]))
