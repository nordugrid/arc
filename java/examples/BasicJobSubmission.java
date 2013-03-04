import nordugrid.arc.Logger;
import nordugrid.arc.LogStream_ostream;
import nordugrid.arc.UserConfig;
import nordugrid.arc.Endpoint;
import nordugrid.arc.EndpointList;
import nordugrid.arc.Job;
import nordugrid.arc.JobList;
import nordugrid.arc.JobDescription;
import nordugrid.arc.JobDescriptionList;
import nordugrid.arc.Submitter;
import nordugrid.arc.JobInformationStorageXML;

public class BasicJobSubmission
{
    public static void main(String argv[])
    {
        // Set up logging to stderr with level VERBOSE (a lot of output will be shown)
        LogStream_ostream logstdout = new LogStream_ostream(nordugrid.arc.arc.getStdout());
        logstdout.setFormat(nordugrid.arc.LogFormat.ShortFormat);
        Logger.getRootLogger().addDestination(logstdout);
        Logger.getRootLogger().setThreshold(nordugrid.arc.LogLevel.VERBOSE);
        Logger logger = new Logger(Logger.getRootLogger(), "jobsubmit");

        // UserConfig contains information on credentials and default services to use.
        // This form of the constructor is necessary to initialise the local job list.
        UserConfig usercfg = new UserConfig("", "");
      
        // Simple job description which outputs hostname to stdout
        String jobdesc = "&(executable=/bin/hostname)(stdout=stdout)";
      
        // Parse job description
        JobDescriptionList jobdescs = new JobDescriptionList();
        if (!JobDescription.Parse(jobdesc, jobdescs).toBool()) {
          logger.msg(nordugrid.arc.LogLevel.ERROR, "Invalid job description");
          System.exit(1);
        }
      
        // Use top-level NorduGrid information index to find resources
        Endpoint index = new Endpoint("ldap://index1.nordugrid.org:2135/Mds-Vo-name=NorduGrid,o=grid",
                                      Endpoint.CapabilityEnum.REGISTRY,
                                      "org.nordugrid.ldapegiis");
        EndpointList services = new EndpointList();
        services.add(index);
      
        // Do the submission
        JobList jobs = new JobList();
        Submitter submitter = new Submitter(usercfg);
        if (!submitter.BrokeredSubmit(services, jobdescs, jobs).equals(nordugrid.arc.SubmissionStatus.SubmissionStatusType.NONE)) {
          logger.msg(nordugrid.arc.LogLevel.ERROR, "Failed to submit job");
          System.exit(1);
        }
      
        // Write information on submitted job to local job list (~/.arc/jobs.xml)
        JobInformationStorageXML jobList = new JobInformationStorageXML(usercfg.JobListFile());
        if (!jobList.Write(jobs)) {
          logger.msg(nordugrid.arc.LogLevel.WARNING, "Failed to write to local job list " + usercfg.JobListFile());
        }
      
        // Job submitted ok
        System.out.println("Job submitted with job id " + jobs.begin().next().getJobID());
        return;
    }
}
