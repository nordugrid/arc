#ifndef __ARC_TARGETGENERATOR_H__
#define __ARC_TARGETGENERATOR_H__

#include <list>
#include <string>

#include <glibmm/thread.h>

#include <arc/client/ExecutionTarget.h>

namespace Arc {

  class Config;
  class Loader;
  class Logger;
  class URL;

  class TargetGenerator {
  public:
    TargetGenerator(Config& cfg);
    ~TargetGenerator();

    void GetTargets(int targetType, int detailLevel);

    bool AddService(const URL& NewService);
    void AddTarget(const ExecutionTarget& NewTarget);
    bool DoIAlreadyExist(const URL& NewServer);

    void PrintTargetInfo(bool longlist);

    std::list<URL> FoundServices;
    std::list<URL> CheckedInfoServers;
    std::list<ExecutionTarget> FoundTargets;

  private:
    Glib::Mutex ServiceMutex;
    Glib::Mutex ServerMutex;
    Glib::Mutex TargetMutex;

    Loader *ACCloader;
    static Logger logger;
  };

} //namespace ARC

#endif // __ARC_TARGETGENERATOR_H__
