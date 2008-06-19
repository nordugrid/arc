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
    void DownloadJobOutput(bool keep, std::string downloaddir);
    void PerformAction(std::string action);

    static ACC *Instance(Config *cfg, ChainContext *cxt);

  private:

    void DownloadThisJob(Job ThisJob, bool keep, std::string downloaddir);
    
  };
  
} //namespace ARC

#endif
