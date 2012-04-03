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
class ThreadedPointer<class Endpoint>;
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
class EndpointQueryOptions<Endpoint> {
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

template<typename T>
class EntityRetrieverPlugin : public Plugin {
protected:
  EntityRetrieverPlugin(PluginArgument* parg): Plugin(parg) {};
public:
  virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterfaces; };
  virtual bool isEndpointNotSupported(const Endpoint&) const = 0;
  virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint& rEndpoint, std::list<T>&, const EndpointQueryOptions<T>& options) const = 0;

  static const std::string kind;

protected:
  std::list<std::string> supportedInterfaces;
};

template<typename T>
class EntityRetrieverPluginLoader : public Loader {
public:
  EntityRetrieverPluginLoader() : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}
  ~EntityRetrieverPluginLoader();

  EntityRetrieverPlugin<T>* load(const std::string& name);
  static std::list<std::string> getListOfPlugins();
  const std::map<std::string, EntityRetrieverPlugin<T> *>& GetTargetInformationRetrieverPlugins() const { return plugins; }

protected:
  std::map<std::string, EntityRetrieverPlugin<T> *> plugins;

  static Logger logger;
};

template<typename T>
class EntityConsumer {
public:
  virtual ~EntityConsumer() {}
  virtual void addEntity(const T&) = 0;
};

template<typename T>
class EntityContainer : public EntityConsumer<T>, public std::list<T> {
public:
  virtual ~EntityContainer() {}
  virtual void addEntity(const T& t) { push_back(t); }
};

template<typename T>
class EntityRetriever : public EntityConsumer<T> {
public:
  EntityRetriever(const UserConfig&, const EndpointQueryOptions<T>& options = EndpointQueryOptions<T>());
  ~EntityRetriever() { common->deactivate(); }

  void wait() const { result.wait(); };
  //void waitForAll() const; // TODO: Make it possible to be nice and wait for all threads to finish.
  bool isDone() const { return result.wait(0); };

  void addConsumer(EntityConsumer<T>& c) { consumerLock.lock(); consumers.push_back(&c); consumerLock.unlock(); };
  void removeConsumer(const EntityConsumer<T>&);

  EndpointQueryingStatus getStatusOfEndpoint(const Endpoint&) const;
  bool setStatusOfEndpoint(const Endpoint&, const EndpointQueryingStatus&, bool overwrite = true);

  virtual void addEntity(const T&);
  virtual void addEndpoint(const Endpoint&);

protected:
  static void queryEndpoint(void *arg_);

  // Common configuration part
  class Common : public EntityRetrieverPluginLoader<T> {
  public:
    Common(EntityRetriever* t, const UserConfig& u) : EntityRetrieverPluginLoader<T>(),
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
    EntityRetriever* operator->(void) { return t; }
    EntityRetriever* operator*(void) { return t; }
  private:
    SharedMutex mutex;
    EntityRetriever* t;
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
    ThreadArg(const ThreadedPointer<Common>& common, Result& result, const Endpoint& endpoint, const EndpointQueryOptions<T>& options) : common(common), result(result), endpoint(endpoint), options(options) {};
    ThreadArg(const ThreadArg& v, Result& result) : common(v.common), result(result), endpoint(v.endpoint), pluginName(v.pluginName), options(options) {};
    // Objects for communication with caller
    ThreadedPointer<Common> common;
    Result result;
    // Per-thread parameters
    Endpoint endpoint;
    std::string pluginName;
    EndpointQueryOptions<T> options;
  };

  std::map<Endpoint, EndpointQueryingStatus> statuses;

  static Logger logger;
  const UserConfig& uc;
  std::list< EntityConsumer<T>* > consumers;
  EndpointQueryOptions<T> options;

  mutable SimpleCondition consumerLock;
  mutable SimpleCondition statusLock;
  std::map<std::string, std::string> interfacePluginMap;
};


typedef EntityRetriever<Endpoint>             ServiceEndpointRetriever;
typedef EntityRetrieverPlugin<Endpoint>       ServiceEndpointRetrieverPlugin;
typedef EntityRetrieverPluginLoader<Endpoint> ServiceEndpointRetrieverPluginLoader;

typedef EntityRetriever<ComputingServiceType>             TargetInformationRetriever;
typedef EntityRetrieverPlugin<ComputingServiceType>       TargetInformationRetrieverPlugin;
typedef EntityRetrieverPluginLoader<ComputingServiceType> TargetInformationRetrieverPluginLoader;

typedef EntityRetriever<Job>             JobListRetriever;
typedef EntityRetrieverPlugin<Job>       JobListRetrieverPlugin;
typedef EntityRetrieverPluginLoader<Job> JobListRetrieverPluginLoader;

} // namespace Arc

#endif // __ARC_ENDPOINTRETRIEVER_H__

