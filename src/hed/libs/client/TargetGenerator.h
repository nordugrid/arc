#ifndef __ARC_TARGETGENERATOR_H__
#define __ARC_TARGETGENERATOR_H__

#include <list>
#include <string>

#include <glibmm/thread.h>

#include <arc/client/ExecutionTarget.h>
#include <arc/loader/Loader.h>

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
    const std::list<ExecutionTarget> FoundTargets() const;

    bool AddService(const URL& url);
    bool AddIndexServer(const URL& url);
    void AddTarget(const ExecutionTarget& target);
    void RetrieverDone();

    void PrintTargetInfo(bool longlist) const;

  private:
    Loader loader;

    std::list<URL> foundServices;
    std::list<URL> foundIndexServers;
    std::list<ExecutionTarget> foundTargets;

    Glib::Mutex serviceMutex;
    Glib::Mutex indexServerMutex;
    Glib::Mutex targetMutex;

    int threadCounter;
    Glib::Mutex threadMutex;
    Glib::Cond threadCond;

    static Logger logger;
  };

} //namespace ARC

#endif // __ARC_TARGETGENERATOR_H__
