/**
 * Base class for bulk job control
 */
#ifndef ARCLIB_JOBCONTROLLER
#define ARCLIB_JOBCONTROLLER

#include <list>
#include <string>
#include <arc/ArcConfig.h>
#include <arc/client/Job.h>

namespace Arc{
  
  class JobController 
    : public ACC {
  protected: 
    JobController(Arc::Config *cfg, std::string flavour);
  public:
    //These should be implemented by specialized class
    virtual void GetJobInformation() = 0;
    virtual void PerformAction(std::string action) = 0;

    //Base class implementation
    void IdentifyJobs(std::list<std::string> jobids);
    void PrintJobInformation(bool longlist);
  protected:
    std::list<Arc::Job> JobStore;
    std::string GridFlavour;
    Arc::Config mcfg;
    virtual ~JobController();
  };

} //namespace ARC

#endif
