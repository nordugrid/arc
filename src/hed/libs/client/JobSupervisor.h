// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/client/JobController.h>

namespace Arc {

  class Logger;
  class UserConfig;

  /// % JobSupervisor class
  /**
   * The JobSupervisor class is tool for loading JobController plugins
   * for managing Grid jobs.
   **/

  class JobSupervisor {
  public:
    /// Create a JobSupervisor object
    /**
     * Default constructor to create a JobSupervisor. Automatically
     * loads JobController plugins based upon the input jobids.
     * 
     * @param usercfg Reference to UserConfig object with information
     * about user credentials and joblistfile.
     *
     * @param jobs List of jobs(jobid or job name) to be managed.
     *
     **/
    JobSupervisor(const UserConfig& usercfg,
                  const std::list<std::string>& jobs);

    ~JobSupervisor();

    /// Get list of JobControllers
    /**
     * Method to get the list of JobControllers loaded by constructor.
     *
     **/
    const std::list<JobController*>& GetJobControllers() {
      return loader.GetJobControllers();
    }

    bool JobsFound() const { return jobsFound; }

  private:
    static Logger logger;
    JobControllerLoader loader;
    bool jobsFound;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
