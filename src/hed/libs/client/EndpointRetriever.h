#ifndef __ARC_ENDPOINTRETRIEVER_H__
#define __ARC_ENDPOINTRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/client/Endpoint.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/loader/Loader.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

class Logger;
class SharedMutex;
class SimpleCondition;
class SimpleCounter;
class ThreadedPointer<class T>;
class UserConfig;

template<typename T>
class EndpointQueryOptions {
public:
  EndpointQueryOptions(std::list<std::string> preferredInterfaceNames = std::list<std::string>()) : preferredInterfaceNames(preferredInterfaceNames) {}
  std::list<std::string>& getPreferredInterfaceNames() { return preferredInterfaceNames; }
private:
  std::list<std::string> preferredInterfaceNames;
};

template<>
class EndpointQueryOptions<ServiceEndpoint> {
public:
  EndpointQueryOptions(bool recursive = false,
                       const std::list<std::string>& capabilityFilter = std::list<std::string>(),
                       const std::list<std::string>& rejectedServices = std::list<std::string>() )
    : recursive(recursive), capabilityFilter(capabilityFilter), rejectedServices(rejectedServices) {}

  bool recursiveEnabled() const { return recursive; }
  const std::list<std::string>& getCapabilityFilter() const { return capabilityFilter; }
  const std::list<std::string>& getRejectedServices() const { return rejectedServices; }
  std::list<std::string>& getPreferredInterfaceNames() { return preferredInterfaceNames; }

private:
  bool recursive;
  std::list<std::string> capabilityFilter;
  std::list<std::string> rejectedServices;
  std::list<std::string> preferredInterfaceNames;
};

template<typename T, typename S>
class EndpointRetrieverPlugin : public Plugin {
protected:
  EndpointRetrieverPlugin() {};
public:
  virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterfaces; };
  virtual bool isEndpointNotSupported(const T&) const = 0;
  virtual EndpointQueryingStatus Query(const UserConfig&, const T& rEndpoint, std::list<S>&, const EndpointQueryOptions<S>& options) const = 0;

  static const std::string kind;

protected:
  std::list<std::string> supportedInterfaces;
};

template<typename T, typename S>
class EndpointRetrieverPluginLoader : public Loader {
public:
  EndpointRetrieverPluginLoader() : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}
  ~EndpointRetrieverPluginLoader();

  EndpointRetrieverPlugin<T, S>* load(const std::string& name);
  static std::list<std::string> getListOfPlugins();
  const std::map<std::string, EndpointRetrieverPlugin<T, S> *>& GetTargetInformationRetrieverPlugins() const { return plugins; }

protected:
  std::map<std::string, EndpointRetrieverPlugin<T, S>*> plugins;

  static Logger logger;
};

template<typename T>
class EndpointConsumer {
public:
  virtual ~EndpointConsumer() {}
  virtual void addEndpoint(const T&) = 0;
};

template<typename T>
class EndpointContainer : public EndpointConsumer<T>, public std::list<T> {
public:
  virtual ~EndpointContainer() {}
  virtual void addEndpoint(const T& t) { push_back(t); }
};

template<typename T, typename S>
class EndpointRetriever : public EndpointConsumer<T>, public EndpointConsumer<S> {
public:
  EndpointRetriever(const UserConfig&, const EndpointQueryOptions<S>& options = EndpointQueryOptions<S>());
  ~EndpointRetriever() { common->deactivate(); }

  void wait() const { result.wait(); };
  //void waitForAll() const; // TODO: Make it possible to be nice and wait for all threads to finish.
  bool isDone() const { return result.wait(0); };

  void addConsumer(EndpointConsumer<S>& c) { consumerLock.lock(); consumers.push_back(&c); consumerLock.unlock(); };
  void removeConsumer(const EndpointConsumer<S>&);

  EndpointQueryingStatus getStatusOfEndpoint(const T&) const;
  bool setStatusOfEndpoint(const T&, const EndpointQueryingStatus&, bool overwrite = true);

  virtual void addEndpoint(const S&);
  virtual void addEndpoint(const T&);

protected:
  static void queryEndpoint(void *arg_);

  // Common configuration part
  class Common : public EndpointRetrieverPluginLoader<T, S> {
  public:
    Common(EndpointRetriever* t, const UserConfig& u) : EndpointRetrieverPluginLoader<T, S>(),
      mutex(), t(t), uc(u) {};
    void deactivate(void) {
      mutex.lockExclusive();
      t = NULL;
      mutex.unlockExclusive();
    }
    bool lockExclusiveIfValid(void) {
      mutex.lockExclusive();
      if(t) return true;
      mutex.unlockExclusive();
      return false;
    }
    void unlockExclusive(void) { mutex.unlockExclusive(); }
    bool lockSharedIfValid(void) {
      mutex.lockShared();
      if(t) return true;
      mutex.unlockShared();
      return false;
    }
    void unlockShared(void) { mutex.unlockShared(); }

    operator const UserConfig&(void) const { return uc; }
    const std::list<std::string>& getAvailablePlugins(void) const { return availablePlugins; }
    void setAvailablePlugins(const std::list<std::string>& newAvailablePlugins) { availablePlugins = newAvailablePlugins; }
    EndpointRetriever* operator->(void) { return t; }
    EndpointRetriever* operator*(void) { return t; }
  private:
    SharedMutex mutex;
    EndpointRetriever* t;
    const UserConfig uc;
    std::list<std::string> availablePlugins;
  };
  ThreadedPointer<Common> common;

  // Represents completeness of queriies run in threads.
  // Different implementations are meant for waiting for either one or all threads.
  // TODO: counter is duplicate in this implimentation. It may be simplified
  //       either by using counter of ThreadedPointer or implementing part of
  //       ThreadedPointer directly.
  class Result: private ThreadedPointer<SimpleCounter>  {
  public:
    // Creates initial instance
    Result(bool one_success = false):
      ThreadedPointer<SimpleCounter>(new SimpleCounter),
      success(false),need_one_success(one_success) { };
    // Creates new reference representing query - increments counter
    Result(const Result& r):
      ThreadedPointer<SimpleCounter>(r),
      success(false),need_one_success(r.need_one_success) {
      Ptr()->inc();
    };
    // Query finished - decrement or reset counter (if one result is enough)
    ~Result(void) {
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
  private:
    bool success;
    bool need_one_success;
  };
  Result result;

  class ThreadArg {
  public:
    ThreadArg(const ThreadedPointer<Common>& common, Result& result, const T& endpoint, const EndpointQueryOptions<S>& options) : common(common), result(result), endpoint(endpoint), options(options) {};
    ThreadArg(const ThreadArg& v, Result& result) : common(v.common), result(result), endpoint(v.endpoint), pluginName(v.pluginName), options(options) {};
    // Objects for communication with caller
    ThreadedPointer<Common> common;
    Result result;
    // Per-thread parameters
    T endpoint;
    std::string pluginName;
    EndpointQueryOptions<S> options;
  };

  std::map<T, EndpointQueryingStatus> statuses;

  static Logger logger;
  const UserConfig& uc;
  std::list< EndpointConsumer<S>* > consumers;
  EndpointQueryOptions<S> options;

  mutable SimpleCondition consumerLock;
  mutable SimpleCondition statusLock;
  std::map<std::string, std::string> interfacePluginMap;
};


typedef EndpointRetriever<RegistryEndpoint, ServiceEndpoint>             ServiceEndpointRetriever;
typedef EndpointRetrieverPlugin<RegistryEndpoint, ServiceEndpoint>       ServiceEndpointRetrieverPlugin;
typedef EndpointRetrieverPluginLoader<RegistryEndpoint, ServiceEndpoint> ServiceEndpointRetrieverPluginLoader;

typedef EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget>             TargetInformationRetriever;
typedef EndpointRetrieverPlugin<ComputingInfoEndpoint, ExecutionTarget>       TargetInformationRetrieverPlugin;
typedef EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget> TargetInformationRetrieverPluginLoader;

typedef EndpointRetriever<ComputingInfoEndpoint, Job>             JobListRetriever;
typedef EndpointRetrieverPlugin<ComputingInfoEndpoint, Job>       JobListRetrieverPlugin;
typedef EndpointRetrieverPluginLoader<ComputingInfoEndpoint, Job> JobListRetrieverPluginLoader;


class ExecutionTargetRetriever : public EndpointConsumer<ServiceEndpoint>, public EndpointContainer<ExecutionTarget> {
public:
  ExecutionTargetRetriever(
    const UserConfig& uc,
    const std::list<ServiceEndpoint>& services,
    const std::list<std::string>& rejectedServices = std::list<std::string>(),
    const std::list<std::string>& preferredInterfaceNames = std::list<std::string>(),
    const std::list<std::string>& capabilityFilter = std::list<std::string>(1, ComputingInfoEndpoint::ComputingInfoCapability)
  );

  void wait();

  void addEndpoint(const ServiceEndpoint& service);

private:
  ServiceEndpointRetriever ser;
  TargetInformationRetriever tir;
};

} // namespace Arc

#endif // __ARC_ENDPOINTRETRIEVER_H__
