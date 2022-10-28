#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/Submitter.h>
#include <arc/compute/JobInformationStorageXML.h>

int main() {

  // Set up logging to stderr with level VERBOSE (a lot of output will be shown)
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);
  Arc::Logger logger(Arc::Logger::getRootLogger(), "jobsubmit");

  // UserConfig contains information on credentials and default services to use.
  // This form of the constructor is necessary to initialise the local job list.
  Arc::UserConfig usercfg("", "");

  // Simple job description which outputs hostname to stdout
  std::string jobdesc("&(executable=/bin/hostname)(stdout=stdout)");

  // Parse job description
  std::list<Arc::JobDescription> jobdescs;
  if (!Arc::JobDescription::Parse(jobdesc, jobdescs)) {
    logger.msg(Arc::ERROR, "Invalid job description");
    return 1;
  }

  /*
   * Use 'Arc::JobDescription::ParseFromFile("helloworld.xrsl", jobdescs)'
   * to parse job description from file.
   */

  // Use top-level NorduGrid information index to find resources
  Arc::Endpoint index("nordugrid.org",
                      Arc::Endpoint::REGISTRY,
                      "org.nordugrid.archery");
  std::list<Arc::Endpoint> services(1, index);

  // Do the submission
  std::list<Arc::Job> jobs;
  Arc::Submitter submitter(usercfg);
  if (submitter.BrokeredSubmit(services, jobdescs, jobs) != Arc::SubmissionStatus::NONE) {
    logger.msg(Arc::ERROR, "Failed to submit job");
    return 1;
  }

  // Write information on submitted job to local job list (~/.arc/jobs.xml)
  Arc::JobInformationStorageXML jobList(usercfg.JobListFile());
  if (!jobList.Write(jobs)) {
    logger.msg(Arc::WARNING, "Failed to write to local job list %s", usercfg.JobListFile());
  }

  // Job submitted ok
  std::cout << "Job submitted with job id " << jobs.front().JobID << std::endl;
  return 0;
}
