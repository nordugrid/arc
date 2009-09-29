// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETGENERATOR_H__
#define __ARC_TARGETGENERATOR_H__

#include <list>
#include <string>


#include <arc/Thread.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Config;
  class Logger;
  class URL;

  class TargetGenerator {
  public:
    TargetGenerator(const UserConfig& usercfg);
    ~TargetGenerator();

    void GetTargets(int targetType, int detailLevel);
    const std::list<ExecutionTarget>& FoundTargets() const;
    std::list<ExecutionTarget>& ModifyFoundTargets();
    const std::list<XMLNode*>& FoundJobs() const;

    bool AddService(const URL& url);
    bool AddIndexServer(const URL& url);
    void AddTarget(const ExecutionTarget& target);
    void AddJob(const XMLNode& job);
    void RetrieverDone();

    void PrintTargetInfo(bool longlist) const;

  private:
    TargetRetrieverLoader loader;

    const UserConfig& usercfg;

    std::list<URL> foundServices;
    std::list<URL> foundIndexServers;
    std::list<ExecutionTarget> foundTargets;
    std::list<XMLNode*> foundJobs;

    Glib::Mutex serviceMutex;
    Glib::Mutex indexServerMutex;
    Glib::Mutex targetMutex;
    Glib::Mutex jobMutex;

    int threadCounter;
    Glib::Mutex threadMutex;
    Glib::Cond threadCond;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETGENERATOR_H__
