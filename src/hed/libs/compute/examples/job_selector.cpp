#include <iostream>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobSupervisor.h>

/*
 * Create a JobSelector class in order to specify a custom selection to be used
 * with the JobSupervisor class.
 */

// Extend the arc.compute.JobSelector class and the select method.
class ThreeDaysOldJobSelector : public Arc::JobSelector {
public:
  ThreeDaysOldJobSelector() {
    now = Arc::Time();
    three_days = Arc::Period(60*60*24*3);
    //three_days = Arc::Period("P3D") // ISO duration
    //three_days = Arc::Period(3*Arc::Time.DAY)
  }

  // The select method recieves a arc.compute.Job instance and must return a
  // boolean, indicating whether the job should be selected or rejected.
  // All attributes of the arc.compute.Job object can be used in this method.
  bool Select(const Arc::Job& job) const {
    return (now - job.EndTime) > three_days;
  }

private:
  Arc::Time now;
  Arc::Period three_days;
};

int main(int argc, char** argv) {
  Arc::UserConfig uc;

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  Arc::Job j;
  j.JobManagementInterfaceName = "org.ogf.glue.emies.activitymanagement";
  j.JobManagementURL = Arc::URL("https://localhost");
  j.JobStatusInterfaceName = "org.ogf.glue.emies.activitymanagement";
  j.JobStatusURL = Arc::URL("https://localhost");

  Arc::JobSupervisor js(uc);

  j.JobID = "test-job-1-day-old";
  j.EndTime = Arc::Time()-Arc::Period("P1D");
  js.AddJob(j);

  j.JobID = "test-job-2-days-old";
  j.EndTime = Arc::Time()-Arc::Period("P2D");
  js.AddJob(j);

  j.JobID = "test-job-3-days-old";
  j.EndTime = Arc::Time()-Arc::Period("P3D");
  js.AddJob(j);

  j.JobID = "test-job-4-days-old";
  j.EndTime = Arc::Time()-Arc::Period("P4D");
  js.AddJob(j);

  ThreeDaysOldJobSelector selector;
  js.Select(selector);

  std::list<Arc::Job> selectedJobs = js.GetSelectedJobs();
  for (std::list<Arc::Job>::iterator itJ = selectedJobs.begin();
       itJ != selectedJobs.end(); ++itJ) {
    std::cout << itJ->JobID << std::endl;
  }

  // Make operation on selected jobs. E.g.:
  //js.Clean()

  return 0;
}
