/**
 * Class for bulk ARC0 job control
 */
#ifndef ARCLIB_JOBARC0CONTROLLER
#define ARCLIB_JOBARC0CONTROLLER

#include <arc/client/JobController.h>

namespace Arc{

  class ChainContext;
  class Config;
  
  class JobControllerARC0 : public JobController {
    
  public: 
    
    JobControllerARC0(Arc::Config *cfg);
    ~JobControllerARC0();
    
    void GetJobInformation();
    static ACC *Instance(Config *cfg, ChainContext *cxt);

  private:
    static Logger logger;
    bool GetThisJob(Job ThisJob, std::string downloaddir);
    bool CleanThisJob(Job ThisJob, bool force);
    bool CancelThisJob(Job ThisJob);
    URL GetFileUrlThisJob(Job ThisJob, std::string whichfile);

  };
  
} //namespace ARC

#endif
