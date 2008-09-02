/**
 * Class for bulk ARC1 job control
 */
#ifndef ARCLIB_JOBARC1CONTROLLER
#define ARCLIB_JOBARC1CONTROLLER

#include <arc/client/JobController.h>

namespace Arc{

  class ChainContext;
  class Config;
  class DataHandle;

  class JobControllerARC1 : public JobController {
    
  public: 
    
    JobControllerARC1(Arc::Config *cfg);
    ~JobControllerARC1();
    
    void GetJobInformation();
    static ACC *Instance(Config *cfg, ChainContext *cxt);
    
  private:
    static Logger logger;
    bool GetThisJob(Job ThisJob, const std::string& downloaddir);
    bool CleanThisJob(Job ThisJob, bool force);
    bool CancelThisJob(Job ThisJob);
    URL GetFileUrlThisJob(Job ThisJob, const std::string& whichfile);
    std::list<std::string> GetDownloadFiles(DataHandle& dir,
					    const std::string& dirname = "");
  };
  
} //namespace ARC

#endif
