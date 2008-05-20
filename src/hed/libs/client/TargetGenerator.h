#ifndef __ARC_TARGETGENERATOR_H__
#define __ARC_TARGETGENERATOR_H__

#include <list>
#include <string>

#include <glibmm/thread.h>

#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/client/ACC.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/loader/Loader.h>

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

#endif // __ARC_TARGETGENERATOR_H__
