#ifndef __ARC_JOBSUPERVISOR_H__
#define __ARC_JOBSUPERVISOR_H__

#include <list>
#include <string>

#include <arc/client/JobController.h>
#include <arc/loader/Loader.h>

namespace Arc {

  class JobSupervisor {
  public:
    JobSupervisor(Arc::XMLNode JobIdStorage, std::list<std::string> jobids);
    ~JobSupervisor();

    void PrintJobInformation(bool longlist);

  private:

    Loader *ACCloader;
    std::list<Arc::JobController*> JobControllers;

  };

} //namespace ARC

#endif // __ARC_JOBSUPERVISOR_H__
