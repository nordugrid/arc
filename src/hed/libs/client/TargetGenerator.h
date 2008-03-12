/**
 * Class for generation of targets
 */
#ifndef ARCLIB_TARGETGENERATOR
#define ARCLIB_TARGETGENERATOR

#include <arc/client/TargetRetriever.h>
#include <arc/ArcConfig.h>
#include <arc/loader/Loader.h>
#include <arc/client/ACC.h>

#include <string>

namespace Arc{
  
  class TargetGenerator{
  public: 
    TargetGenerator(Arc::Config &cfg);
    ~TargetGenerator();
    
    std::list<ACC*> getTargets();
    
  private:

    Arc::Loader *ACCloader;
    
  };
  
} //namespace ARC

#endif
