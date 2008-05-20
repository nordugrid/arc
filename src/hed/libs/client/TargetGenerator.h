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

namespace Arc {

  class TargetGenerator {
  public:
    TargetGenerator(Config& cfg);
    ~TargetGenerator();

    void GetTargets(int TargetType, int DetailLevel);

    bool AddService(std::string NewService);
    void AddTarget(ExecutionTarget NewTarget);
    bool DoIAlreadyExist(std::string NewServer);

    void PrintTargetInfo(bool longlist);

    std::list<std::string> FoundServices;
    std::list<std::string> CheckedInfoServers;
    std::list<ExecutionTarget> FoundTargets;

  private:
    Glib::Mutex ServiceMutex;
    Glib::Mutex ServerMutex;
    Glib::Mutex TargetMutex;

    Loader *ACCloader;

  };

} //namespace ARC

#endif
