/**
 * Class for generation of targets
 */
#ifndef ARCLIB_TARGETGENERATOR
#define ARCLIB_TARGETGENERATOR

#include <arc/ArcConfig.h>
#include <arc/loader/Loader.h>
#include <arc/client/ACC.h>
#include <arc/client/ExecutionTarget.h>
#include <glibmm/thread.h>
#include <arc/URL.h>
#include <string>
#include <list>

namespace Arc{
  
  class TargetGenerator{
  public: 
    TargetGenerator(Arc::Config &cfg);
    ~TargetGenerator();
    
    void GetTargets();

    bool AddService(Arc::URL NewService);
    void AddTarget(Arc::ExecutionTarget NewTarget);    
    bool DoIAlreadyExist(Arc::URL NewServer);
    
    std::list<Arc::URL> FoundServices;
    std::list<Arc::URL> CheckedInfoServers;
    std::list<Arc::ACC> FoundTargets; 
    
  private:
    Glib::Mutex ServiceMutex;
    Glib::Mutex ServerMutex;
    Glib::Mutex TargetMutex;
    
    Arc::Loader *ACCloader;

  };
  
} //namespace ARC

#endif
