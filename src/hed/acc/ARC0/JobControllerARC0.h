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
    void Clean(bool force);
    void Kill(bool keep);

    static ACC *Instance(Config *cfg, ChainContext *cxt);

  private:
    static Logger logger;
    bool DownloadThisJob(Job ThisJob, bool keep, std::string downloaddir);
    bool CleanThisJob(Job ThisJob, bool force);
    
  };
  
} //namespace ARC

#endif
