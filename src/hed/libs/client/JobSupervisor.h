#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

#include <arc/client/JobController.h>
#include <arc/loader/Loader.h>
#include <arc/Logger.h>

namespace Arc {

  class JobSupervisor {
  public:
    JobSupervisor(std::string joblist, std::list<std::string> jobids);

    JobSupervisor(const std::list<std::string>& jobs,
		  const std::list<std::string>& clusterselect,
		  const std::list<std::string>& clusterreject,
		  const std::list<std::string>& status,
		  const std::string downloaddir,
		  const std::string joblist,
		  const bool keep,
		  const bool force,
		  const int timeout);
            
    ~JobSupervisor();

    void GetJobInformation();
    void DownloadJobOutput();
    void Clean();
    void Kill();
    void PrintJobInformation(bool longlist);

    const std::list<std::string> clusterselect;
    const std::list<std::string> clusterreject;
    const std::list<std::string> status;
    const std::string downloaddir;
    const bool keep;
    const bool force;
    const int timeout;

  private:
    static Logger logger;
    Loader *ACCloader;
    std::list<Arc::JobController*> JobControllers;

  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
