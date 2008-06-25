/**
 * Class for bulk ARC1 job control
 */
#ifndef ARCLIB_JOBARC1CONTROLLER
#define ARCLIB_JOBARC1CONTROLLER

#include <arc/client/JobController.h>

namespace Arc{

  class ChainContext;
  class Config;
  
  class JobControllerARC1 : public JobController {
    
  private: 
    
    JobControllerARC1(Arc::Config *cfg);
    ~JobControllerARC1();
    
  public:

    void GetJobInformation();
    void DownloadJobOutput(bool keep, std::string downloaddir);
    void Clean(bool force);

    static ACC *Instance(Config *cfg, ChainContext *cxt);
    
  };
  
} //namespace ARC

#endif
