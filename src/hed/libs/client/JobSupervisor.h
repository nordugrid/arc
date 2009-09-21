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

  class JobSupervisor {
  public:
    JobSupervisor(const UserConfig& usercfg,
                  const std::list<std::string>& jobs,
                  const std::list<std::string>& clusters);

    ~JobSupervisor();

    const std::list<JobController*>& GetJobControllers() {
      return loader.GetJobControllers();
    }

  private:
    static Logger logger;
    JobControllerLoader loader;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
