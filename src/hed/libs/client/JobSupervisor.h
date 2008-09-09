#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

namespace Arc {

  class JobController;
  class Loader;
  class Logger;
  class UserConfig;

  class JobSupervisor {
  public:
    JobSupervisor(std::string joblist, std::list<std::string> jobids);
    
    JobSupervisor(const UserConfig& ucfg,
		  const std::list<std::string>& jobs,
		  const std::list<std::string>& clusterselect,
		  const std::list<std::string>& clusterreject,
		  const std::string joblist);
    
    ~JobSupervisor();
    
    std::list<Arc::JobController*> GetJobControllers(){return JobControllers;}
    
  private:
    static Logger logger;
    Loader *loader;
    std::list<Arc::JobController*> JobControllers;
  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
