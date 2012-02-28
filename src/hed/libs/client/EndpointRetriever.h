#ifndef __ARC_ENDPOINTRETRIEVER_H__
#define __ARC_ENDPOINTRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/loader/Loader.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

class ExecutionTarget;
class Logger;
class SharedMutex;
class SimpleCondition;
class SimpleCounter;
class ThreadedPointer<class T>;
class UserConfig;

template<typename T>
class EndpointFilter {};

template<typename T, typename S>
class EndpointRetrieverPlugin : public Plugin {
protected:
  EndpointRetrieverPlugin() {};
public:
  virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterfaces; };
  virtual EndpointQueryingStatus Query(const UserConfig&, const T& rEndpoint, std::list<S>&, const EndpointFilter<S>& filter) const = 0;

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
  std::list<std::string> getListOfPlugins();
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
  EndpointRetriever(const UserConfig&, const EndpointFilter<S>& filter = EndpointFilter<S>());
  ~EndpointRetriever() { common->deactivate(); }

  void wait() const { result.wait(); };
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
    EndpointRetriever* operator->(void) { return t; }
    EndpointRetriever* operator*(void) { return t; }
  private:
    SharedMutex mutex;
    EndpointRetriever* t;
    const UserConfig uc;
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
    ThreadArg(const ThreadedPointer<Common>& common, Result& result, const T& endpoint, const EndpointFilter<S>& filter) : common(common), result(result), endpoint(endpoint), filter(filter) {};
    ThreadArg(const ThreadArg& v, Result& result) : common(v.common), result(result), endpoint(v.endpoint), pluginName(v.pluginName), filter(filter) {};
    // Objects for communication with caller
    ThreadedPointer<Common> common;
    Result result;
    // Per-thread parameters
    T endpoint;
    std::string pluginName;
    EndpointFilter<S> filter;
  };

  std::map<T, EndpointQueryingStatus> statuses;

  static Logger logger;
  const UserConfig& uc;
  std::list< EndpointConsumer<S>* > consumers;
  EndpointFilter<S> filter;

  mutable SimpleCondition consumerLock;
  mutable SimpleCondition statusLock;
  std::map<std::string, std::string> interfacePluginMap;
};


  template<typename T, typename S>
  EndpointRetrieverPluginLoader<T, S>::~EndpointRetrieverPluginLoader() {
    for (typename std::map<std::string, EndpointRetrieverPlugin<T, S> *>::iterator it = plugins.begin();
         it != plugins.end(); it++) {
      delete it->second;
    }
  }

  template<typename T, typename S>
  EndpointRetrieverPlugin<T, S>* EndpointRetrieverPluginLoader<T, S>::load(const std::string& name) {
    if (plugins.find(name) != plugins.end()) {
      logger.msg(DEBUG, "Found %s %s (it was loaded already)", EndpointRetrieverPlugin<T, S>::kind, name);
      return plugins[name];
    }

    if (name.empty()) {
      return NULL;
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(), EndpointRetrieverPlugin<T, S>::kind, name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for %s plugin is installed", name, name);
      logger.msg(DEBUG, "%s plugin \"%s\" not found.", EndpointRetrieverPlugin<T, S>::kind, name, name);
      return NULL;
    }

     EndpointRetrieverPlugin<T, S> *p = factory_->GetInstance< EndpointRetrieverPlugin<T, S> >(EndpointRetrieverPlugin<T, S>::kind, name, NULL, false);

    if (!p) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "%s %s could not be created.", EndpointRetrieverPlugin<T, S>::kind, name, name);
      return NULL;
    }

    plugins[name] = p;
    logger.msg(DEBUG, "Loaded % %s", EndpointRetrieverPlugin<T, S>::kind, name);
    return p;
  }

  template<typename T, typename S>
  std::list<std::string> EndpointRetrieverPluginLoader<T, S>::getListOfPlugins() {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind(EndpointRetrieverPlugin<T, S>::kind, modules);
    std::list<std::string> names;
    for (std::list<ModuleDesc>::const_iterator it = modules.begin(); it != modules.end(); it++) {
      for (std::list<PluginDesc>::const_iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); it2++) {
        names.push_back(it2->name);
      }
    }
    return names;
  }

  template<typename T, typename S>
  EndpointRetriever<T, S>::EndpointRetriever(const UserConfig& uc, const EndpointFilter<S>& filter)
    : common(new Common(this, uc)),
      result(),
      uc(uc),
      filter(filter)
  {
    // Used for holding names of all available plugins.
    std::list<std::string> types(common->getListOfPlugins());
    // Map supported interfaces to available plugins.
    for (std::list<std::string>::const_iterator itT = types.begin(); itT != types.end(); ++itT) {
      EndpointRetrieverPlugin<T, S>* p = common->load(*itT);
      if (p) {
        for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
          // If two plugins supports two identical interfaces, then only the last will appear in the map.
          interfacePluginMap[*itI] = *itT;
        }
      }
    }
  }

  template<typename T, typename S>
  void EndpointRetriever<T, S>::removeConsumer(const EndpointConsumer<S>& consumer) {
    consumerLock.lock();
    typename std::list< EndpointConsumer<S>* >::iterator it = std::find(consumers.begin(), consumers.end(), &consumer);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
    consumerLock.unlock();
  }

  template<typename T, typename S>
  void EndpointRetriever<T, S>::addEndpoint(const T& endpoint) {
    std::map<std::string, std::string>::const_iterator itPluginName = interfacePluginMap.end();
    if (!endpoint.InterfaceName.empty()) {
      itPluginName = interfacePluginMap.find(endpoint.InterfaceName);
      if (itPluginName == interfacePluginMap.end()) {
        //logger.msg(DEBUG, "Unable to find TargetInformationRetrieverPlugin plugin to query interface \"%s\" on \"%s\"", endpoint.InterfaceName, endpoint.Endpoint);
        setStatusOfEndpoint(endpoint, EndpointQueryingStatus(EndpointQueryingStatus::NOPLUGIN));
        return;
      }
    }
    // common will be copied into the thread arg,
    // which means that all threads will have a new
    // instance of the ThreadedPointer pointing to the same object
    ThreadArg *arg = new ThreadArg(common, result, endpoint, filter);
    if (itPluginName != interfacePluginMap.end()) {
      arg->pluginName = itPluginName->second;
    }
    logger.msg(Arc::DEBUG, "Starting thread to query the endpoint on %s", arg->endpoint.str());
    if (!CreateThreadFunction(&queryEndpoint, arg)) {
      logger.msg(Arc::ERROR, "Failed to start querying the endpoint on %s", arg->endpoint.str() + " (unable to create thread)");
      setStatusOfEndpoint(endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
      delete arg;
    }
  }

  template<typename T, typename S>
  void EndpointRetriever<T, S>::addEndpoint(const S& s) {
    consumerLock.lock();
    for (typename std::list< EndpointConsumer<S>* >::iterator it = consumers.begin(); it != consumers.end(); it++) {
      (*it)->addEndpoint(s);
    }
    consumerLock.unlock();
  }

  template<typename T, typename S>
  EndpointQueryingStatus EndpointRetriever<T, S>::getStatusOfEndpoint(const T& endpoint) const {
    statusLock.lock();
    EndpointQueryingStatus status(EndpointQueryingStatus::UNKNOWN);
    typename std::map<T, EndpointQueryingStatus>::const_iterator it = statuses.find(endpoint);
    if (it != statuses.end()) {
      status = it->second;
    }
    statusLock.unlock();
    return status;
  }

  template<typename T, typename S>
  bool EndpointRetriever<T, S>::setStatusOfEndpoint(const T& endpoint, const EndpointQueryingStatus& status, bool overwrite) {
    statusLock.lock();
    bool wasSet = false;
    if (overwrite || (statuses.find(endpoint) == statuses.end())) {
      logger.msg(DEBUG, "Setting status (%s) for endpoint: %s", status.str(), endpoint.str());
      statuses[endpoint] = status;
      wasSet = true;
    }
    statusLock.unlock();
    return wasSet;
  };

  template<typename T, typename S>
  void EndpointRetriever<T, S>::queryEndpoint(void *arg) {
    AutoPointer<ThreadArg> a((ThreadArg*)arg);
    ThreadedPointer<Common>& common = a->common;
    bool set = false;
    // Set the status of the endpoint to STARTED only if it was not set already by an other thread (overwrite = false)
    if(!common->lockSharedIfValid()) return;
    set = (*common)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::STARTED), false);
    common->unlockShared();

    if (!set) { // The thread was not able to set the status (because it was already set by another thread)
      logger.msg(DEBUG, "Will not query endpoint (%s) because another thread is already querying it", a->endpoint.str());
      return;
    }
    // If the thread was able to set the status, then this is the first (and only) thread querying this endpoint
    if (!a->pluginName.empty()) { // If the plugin was already selected
      EndpointRetrieverPlugin<T, S>* plugin = common->load(a->pluginName);
      if (!plugin) {
        if(!common->lockSharedIfValid()) return;
        (*common)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
        common->unlockShared();
        return;
      }
      logger.msg(DEBUG, "Calling plugin %s to query endpoint on %s", a->pluginName, a->endpoint.str());
      std::list<S> endpoints;
      // Do the actual querying against service.
      EndpointQueryingStatus status = plugin->Query(*common, a->endpoint, endpoints, a->filter);
      for (typename std::list<S>::const_iterator it = endpoints.begin(); it != endpoints.end(); it++) {
        if(!common->lockSharedIfValid()) return;
        (*common)->addEndpoint(*it);
        common->unlockShared();
      }

      if(!common->lockSharedIfValid()) return;
      (*common)->setStatusOfEndpoint(a->endpoint, status);
      common->unlockShared();
      if (status) a->result.setSuccess(); // Successful query
    } else { // If there was no plugin selected for this endpoint, this will try all possibility
      logger.msg(DEBUG, "The interface of this endpoint (%s) is unspecified, will try all possible plugins", a->endpoint.str());
      std::list<std::string> types = common->getListOfPlugins();
      // A list for collecting the new endpoints which will be created by copying the original one
      // and setting the InterfaceName for each possible plugins
      std::list<T> newEndpoints;
      // A new result object is created for the sub-threads, "true" means we only want to wait for the first successful query
      Result newResult(true);
      for (std::list<std::string>::const_iterator it = types.begin(); it != types.end(); ++it) {
        EndpointRetrieverPlugin<T, S>* plugin = common->load(*it);
        if (!plugin) {
          // Problem loading the plugin, skip it
          logger.msg(DEBUG, "Problem loading plugin %s, skipping it..", *it);
          continue;
        }
        std::list<std::string> interfaceNames = plugin->SupportedInterfaces();
        if (interfaceNames.empty()) {
          // This plugin does not support any interfaces, skip it
          logger.msg(DEBUG, "The plugin %s does not support any interfaces, skipping it.", *it);
          continue;
        } else if (interfaceNames.front().empty()) {
          logger.msg(DEBUG, "The first supported interface of the plugin %s is an empty string, skipping the plugin.", *it);
          continue;
        }
        // Create a new endpoint with the same endpoint and a specified interface
        T endpoint = a->endpoint;
        // We will use the first interfaceName this plugin supports
        endpoint.InterfaceName = interfaceNames.front();
        logger.msg(DEBUG, "New endpoint is created (%s) from the one with the unspecified interface (%s)",  endpoint.str(), a->endpoint.str());
        newEndpoints.push_back(endpoint);
        // Make new argument by copying old one with result report object replaced
        ThreadArg* newArg = new ThreadArg(*a, newResult);
        newArg->endpoint = endpoint;
        newArg->pluginName = *it;
        logger.msg(DEBUG, "Starting sub-thread to query the endpoint on %s", endpoint.str());
        if (!CreateThreadFunction(&queryEndpoint, newArg)) {
          logger.msg(ERROR, "Failed to start querying the endpoint on %s (unable to create sub-thread)", endpoint.str());
          delete newArg;
          if(!common->lockSharedIfValid()) return;
          (*common)->setStatusOfEndpoint(endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
          common->unlockShared();
          continue;
        }
      }
      // We wait for the new result object. The wait returns in two cases:
      //   1. one sub-thread was succesful
      //   2. all the sub-threads failed
      newResult.wait();
      // Check which case happened
      if(!common->lockSharedIfValid()) return;
      size_t failedCount = 0;
      bool wasSuccesful = false;
      for (typename std::list<T>::const_iterator it = newEndpoints.begin(); it != newEndpoints.end(); it++) {
        EndpointQueryingStatus status = (*common)->getStatusOfEndpoint(*it);
        if (status) {
          wasSuccesful = true;
          break;
        } else if (status == EndpointQueryingStatus::FAILED) {
          failedCount++;
        }
      }
      // Set the status of the original endpoint (the one without the specified interface)
      if (wasSuccesful) {
        (*common)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::SUCCESSFUL));
      } else if (failedCount == newEndpoints.size()) {
        (*common)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
      } else {
        (*common)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::UNKNOWN));
      }
      common->unlockShared();
    }
  }

} // namespace Arc

#endif // __ARC_ENDPOINTRETRIEVER_H__
