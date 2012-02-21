#ifndef __ARC_TARGETINFORMATIONRETRIEVER_H__
#define __ARC_TARGETINFORMATIONRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include "TargetInformationRetrieverPlugin.h"
#include <arc/client/ExecutionTarget.h>

namespace Arc {

class EndpointQueryingStatus;
class Logger;
class SharedMutex;
class SimpleCondition;
class SimpleCounter;
class ThreadedPointer<class T>;
class UserConfig;

class ExecutionTargetConsumer {
public:
  virtual void addExecutionTarget(const ExecutionTarget&) = 0;
};

class ExecutionTargetContainer : public ExecutionTargetConsumer {
public:
  void addExecutionTarget(const ExecutionTarget& target) { targets.push_back(target); }
  std::list<ExecutionTarget> targets;  
};

class ComputingInfoEndpoint {
public:
  std::string str() const {
    return Endpoint + " (" + (InterfaceName.empty() ? "<unspecified>" : InterfaceName) + ")";
  }

  // Needed for std::map ('statuses' in ServiceEndpointRetriever) to be able to sort the keys
  bool operator<(const ComputingInfoEndpoint& other) const {
    return Endpoint + InterfaceName < other.Endpoint + other.InterfaceName;
  }

  std::string Endpoint;
  std::string InterfaceName;
};

class ComputingInfoEndpointConsumer {
public:
  virtual void addComputingInfoEndpoint(const ComputingInfoEndpoint&) = 0;
};

class TargetInformationRetriever : public ComputingInfoEndpointConsumer, public ExecutionTargetConsumer {
public:
  TargetInformationRetriever(const UserConfig&);
  ~TargetInformationRetriever() { tirCommon->deactivate(); }

  void wait() const { tirResult.wait(); };
  bool isDone() const { return tirResult.wait(0); };

  void addConsumer(ExecutionTargetConsumer& c) { consumerLock.lock(); consumers.push_back(&c); consumerLock.unlock(); };
  void removeConsumer(const ExecutionTargetConsumer&);

  EndpointQueryingStatus getStatusOfEndpoint(ComputingInfoEndpoint) const;
  bool setStatusOfEndpoint(const ComputingInfoEndpoint&, const EndpointQueryingStatus&, bool overwrite = true);

  virtual void addExecutionTarget(const ExecutionTarget&);
  virtual void addComputingInfoEndpoint(const ComputingInfoEndpoint&);

private:
  static void queryEndpoint(void *arg_);

  // Common configuration part
  class TIRCommon : public TargetInformationRetrieverPluginLoader {
  public:
    TIRCommon(TargetInformationRetriever* t, const UserConfig& u) : TargetInformationRetrieverPluginLoader(),
      mutex(), tir(t), uc(u) {}; // Maybe full copy of UserConfig wouldbe safer?
    void deactivate(void) {
      mutex.lockExclusive();
      tir = NULL;
      mutex.unlockExclusive();
    }
    bool lockExclusiveIfValid(void) {
      mutex.lockExclusive();
      if(tir) return true;
      mutex.unlockExclusive();
      return false;
    }
    void unlockExclusive(void) { mutex.unlockExclusive(); }
    bool lockSharedIfValid(void) {
      mutex.lockShared();
      if(tir) return true;
      mutex.unlockShared();
      return false;
    }
    void unlockShared(void) { mutex.unlockShared(); }

    operator const UserConfig&(void) const { return uc; }
    TargetInformationRetriever* operator->(void) { return tir; }
    TargetInformationRetriever* operator*(void) { return tir; }
  private:
    SharedMutex mutex;
    TargetInformationRetriever* tir;
    const UserConfig& uc;
  };
  ThreadedPointer<TIRCommon> tirCommon;

  // Represents completeness of queriies run in threads.
  // Different implementations are meant for waiting for either one or all threads.
  // TODO: counter is duplicate in this implimentation. It may be simplified
  //       either by using counter of ThreadedPointer or implementing part of
  //       ThreadedPointer directly.
  class TIRResult: private ThreadedPointer<SimpleCounter>  {
  public:
    // Creates initial instance
    TIRResult(bool one_success = false):
      ThreadedPointer<SimpleCounter>(new SimpleCounter),
      success(false),need_one_success(one_success) { };
    // Creates new reference representing query - increments counter
    TIRResult(const TIRResult& r):
      ThreadedPointer<SimpleCounter>(r),
      success(false),need_one_success(r.need_one_success) {
      Ptr()->inc();
    };
    // Query finished - decrement or reset counter (if one result is enough)
    ~TIRResult(void) {
      if(need_one_success && success) {
        Ptr()->set(0);
      } else {
        Ptr()->dec();
      };
    };
    // Mark this result as successful (failure by default)
    void setSuccess(void) { success = true; };
    // Wait for queries to finish
    bool wait(int t = -1) const { return Ptr()->wait(t); };
    int get(void) { return Ptr()->get(); };
  private:
    bool success;
    bool need_one_success;
  };

  TIRResult tirResult;

  class ThreadArgTIR {
  public:
    ThreadArgTIR(const ThreadedPointer<TIRCommon>& TIRCommon,
                 TIRResult& tirResult)
      : tirCommon(tirCommon), tirResult(tirResult) {};
    ThreadArgTIR(const ThreadArgTIR& v,
                 TIRResult& tirResult)
      : tirCommon(v.tirCommon), tirResult(tirResult),
        endpoint(v.endpoint), pluginName(v.pluginName) {};
    // Objects for communication with caller
    ThreadedPointer<TIRCommon> tirCommon;
    TIRResult tirResult;
    // Per-thread parameters
    ComputingInfoEndpoint endpoint;
    std::string pluginName;
  };

  std::map<ComputingInfoEndpoint, EndpointQueryingStatus> statuses;

  static Logger logger;
  const UserConfig& uc;
  std::list<ExecutionTargetConsumer*> consumers;

  mutable SimpleCondition consumerLock;
  mutable SimpleCondition statusLock;
  std::map<std::string, std::string> interfacePluginMap;
};

class TargetInformationRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<ExecutionTarget> etList;
  static EndpointQueryingStatus status;
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVER_H__
