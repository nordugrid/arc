// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

#include <arc/URL.h>

namespace Arc {

  class JobController;
  class ACCLoader;
  class Logger;
  class UserConfig;

  class JobSupervisor {
  public:
    JobSupervisor(const UserConfig& usercfg,
                  const std::list<std::string>& jobs,
                  const std::list<std::string>& clusters,
                  const std::string& joblist);

    ~JobSupervisor();

    const std::list<Arc::JobController*>& GetJobControllers() {
      return jobcontrollers;
    }

  private:
    static Logger logger;
    ACCLoader *loader;
    std::list<Arc::JobController*> jobcontrollers;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
