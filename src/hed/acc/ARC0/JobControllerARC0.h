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
    
  private: 
    
    JobControllerARC0(Arc::Config *cfg);
    ~JobControllerARC0();
    
  public:

    void GetJobInformation();
    void PerformAction(std::string action);

    static ACC *Instance(Config *cfg, ChainContext *cxt);
    
  };
  
} //namespace ARC

#endif
