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

  /// %JobSupervisor for loading JobControllers
  /**
   * The JobSupervisor loads JobControllers for managing Grid jobs.
   **/
  class JobSupervisor {
  public:


    JobSupervisor(const UserConfig& usercfg,
                  const std::list<std::string>& jobs);

    ~JobSupervisor();

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
