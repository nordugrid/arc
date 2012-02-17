// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

  Logger ServiceEndpointRetriever::logger(Logger::getRootLogger(), "ServiceEndpointRetriever");

  std::string string(SERStatus s){
    if      (s == SER_UNKNOWN)     return "SER_UNKNOWN";
    else if (s == SER_STARTED)     return "SER_STARTED";
    else if (s == SER_FAILED)      return "SER_FAILED";
    else if (s == SER_NOPLUGIN)    return "SER_NOPLUGIN";
    else if (s == SER_SUCCESSFUL)  return "SER_SUCCESSFUL";
    else                           return ""; // There should be no other alternative!
  }

  bool ServiceEndpointRetriever::createThread(const RegistryEndpoint& registry) {
    std::map<std::string, std::string>::const_iterator itPluginName = interfacePluginMap.end();
    if (!registry.InterfaceName.empty()) {
      itPluginName = interfacePluginMap.find(registry.InterfaceName);
      if (itPluginName == interfacePluginMap.end()) {
        logger.msg(DEBUG, "Unable to find ServiceRetrieverPlugin plugin to query interface \"%s\" on \"%s\"", registry.InterfaceName, registry.Endpoint);
        setStatusOfRegistry(registry, RegistryEndpointStatus(SER_NOPLUGIN));
        return false;
      }
    }
    // serCommon and threadCounter will be copied into the thread arg,
    // which means that all threads will have a new instance of the ThreadedPointer pointing to the same object
    ThreadArgSER *arg = new ThreadArgSER(uc, serCommon, threadCounter);
    arg->registry = registry;
    if (itPluginName != interfacePluginMap.end()) {
      arg->pluginName = itPluginName->second;
    }
    arg->ser = this;
    logger.msg(Arc::DEBUG, "Starting thread to query the registry on " + arg->registry.str());
    threadCounter->inc();
    if (!CreateThreadFunction(&queryRegistry, arg)) {
      logger.msg(Arc::ERROR, "Failed to start querying the registry on " + arg->registry.str() + " (unable to create thread)");
      threadCounter->dec();
      delete arg;
      return false;
    }
    return true;
  }

  ServiceEndpointRetriever::ServiceEndpointRetriever(const UserConfig& uc,
                                                     std::list<RegistryEndpoint> registries,
                                                     ServiceEndpointConsumer& consumer,
                                                     bool recursive,
                                                     std::list<std::string> capabilityFilter)
    : serCommon(new SERCommon),
      uc(uc),
      consumer(consumer),
      recursive(recursive),
      capabilityFilter(capabilityFilter),
      threadCounter(new SimpleCounter)
  {
    // Used for holding names of all available plugins.
    std::list<std::string> types(serCommon->loader.getListOfPlugins());
    // Map supported interfaces to available plugins.
    for (std::list<std::string>::const_iterator itT = types.begin(); itT != types.end(); ++itT) {
      ServiceEndpointRetrieverPlugin* p = serCommon->loader.load(*itT);
      if (p) {
        for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
          // If two plugins supports two identical interfaces, then only the last will appear in the map.
          interfacePluginMap[*itI] = *itT;
        }
      }
    }

    for (std::list<RegistryEndpoint>::const_iterator it = registries.begin(); it != registries.end(); ++it) {
      createThread(*it);
    }
  }

  ServiceEndpointRetriever::~ServiceEndpointRetriever() {
    if (serCommon->isActive) {
      serCommon->mutex.lockExclusive();
      serCommon->isActive = false;
      serCommon->mutex.unlockExclusive();
    }
  }

  void ServiceEndpointRetriever::stopSendingEndpoints() {
    serCommon->mutex.lockExclusive();
    serCommon->isActive = false;
    serCommon->mutex.unlockExclusive();
  }

  void ServiceEndpointRetriever::addServiceEndpoint(const ServiceEndpoint& endpoint) {
    if (recursive && RegistryEndpoint::isRegistry(endpoint)) {
      RegistryEndpoint registry(endpoint);
      logger.msg(Arc::DEBUG, "Found a registry, will query it recursively: " + registry.str());
      createThread(registry);
    }

    bool match = false;
    for (std::list<std::string>::iterator it = capabilityFilter.begin(); it != capabilityFilter.end(); it++) {
      if (std::find(endpoint.EndpointCapabilities.begin(), endpoint.EndpointCapabilities.end(), *it) != endpoint.EndpointCapabilities.end()) {
        match = true;
        break;
      }
    }
    if (capabilityFilter.empty() || match) {
      lock.lock();
      consumer.addServiceEndpoint(endpoint);
      lock.unlock();
    }
  }

  void ServiceEndpointRetriever::wait() const {
    threadCounter->wait();
  };

  bool ServiceEndpointRetriever::isDone() const {
    return threadCounter->get() == 0;
  };

  RegistryEndpointStatus ServiceEndpointRetriever::getStatusOfRegistry(RegistryEndpoint registry) const {
    lock.lock();
    RegistryEndpointStatus status(SER_UNKNOWN);
    std::map<RegistryEndpoint, RegistryEndpointStatus>::const_iterator it = statuses.find(registry);
    if (it != statuses.end()) {
      status = it->second;
    }
    lock.unlock();
    return status;
  }

  bool ServiceEndpointRetriever::setStatusOfRegistry(const RegistryEndpoint& registry, const RegistryEndpointStatus& status, bool overwrite) {
    lock.lock();
    bool wasSet = false;
    if (overwrite || (statuses.find(registry) == statuses.end())) {
      logger.msg(DEBUG, "Setting status (%s) for registry: %s", string(status.status), registry.str());
      statuses[registry] = status;
      wasSet = true;
    }
    lock.unlock();
    return wasSet;
  };

  void ServiceEndpointRetriever::queryRegistry(void *arg) {
    ThreadArgSER* a = (ThreadArgSER*)arg;
    ThreadedPointer<SERCommon>& serCommon = a->serCommon;
    /* lock */ serCommon->mutex.lockShared(); if (!serCommon->isActive) { serCommon->mutex.unlockShared(); a->threadCounter->dec(); delete a; return; }
    // Set the status of the registry to STARTED only if it was not set already by an other thread (overwrite = false)
    bool set = a->ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_STARTED), false);
    /* unlock */ serCommon->mutex.unlockShared();
    if (!set) { // The thread was not able to set the status (because it was already set by another thread)
      logger.msg(DEBUG, "Will not query registry, because another thread is already querying it: " + a->registry.str());
    } else { // If the thread was able to set the status, then this is the first (and only) thread querying this registry
      if (!a->pluginName.empty()) { // If the plugin was already selected
        ServiceEndpointRetrieverPlugin* plugin = serCommon->loader.load(a->pluginName);
        if (plugin) {
          logger.msg(DEBUG, "Calling plugin to query registry on " + a->registry.str());
          std::list<ServiceEndpoint> endpoints;
          RegistryEndpointStatus status = plugin->Query(a->uc, a->registry, endpoints);
          for (std::list<ServiceEndpoint>::iterator it = endpoints.begin(); it != endpoints.end(); it++) {
            /* lock */ serCommon->mutex.lockShared(); if (!serCommon->isActive) { serCommon->mutex.unlockShared(); a->threadCounter->dec(); delete a; return; }
            a->ser->addServiceEndpoint(*it);
            /* unlock */ serCommon->mutex.unlockShared();
          }
          /* lock */ serCommon->mutex.lockShared(); if (!serCommon->isActive) { serCommon->mutex.unlockShared(); a->threadCounter->dec(); delete a; return; }
          a->ser->setStatusOfRegistry(a->registry, status);
          if (status.status == SER_SUCCESSFUL && a->subthread) {
            // A sub-thread should set the counter to zero if the query was succesful
            // because the other sub-threads are not interesting anymore (it's enough the query a registry on one interface)
            a->threadCounter->set(0);
          }
          /* unlock */ serCommon->mutex.unlockShared();
        } else { // If loading the plugin failed
          RegistryEndpointStatus failed(SER_FAILED);
          /* lock */ serCommon->mutex.lockShared(); if (!serCommon->isActive) { serCommon->mutex.unlockShared(); a->threadCounter->dec(); delete a; return; }
          a->ser->setStatusOfRegistry(a->registry, failed);
          /* unlock */ serCommon->mutex.unlockShared();
        }
      } else { // If there was no plugin selected for this registry, this will try all possibility
        logger.msg(DEBUG, "The interface of this registry endpoint is unspecified, will try all possible plugins: " + a->registry.str());
        std::list<std::string> types = serCommon->loader.getListOfPlugins();
        // A list for collecting the new registry endpoints which will be created by copying the original one
        // and setting the InterfaceName for each possible plugins
        std::list<RegistryEndpoint> newRegistries;
        // A new counter is needed for the subthreads
        ThreadedPointer<SimpleCounter> subthreadCounter(new SimpleCounter);
        for (std::list<std::string>::const_iterator it = types.begin(); it != types.end(); ++it) {
          ServiceEndpointRetrieverPlugin* plugin = serCommon->loader.load(*it);
          if (!plugin) {
            // Problem loading the plugin, skip it
            break;
          }
          std::list<std::string> interfaceNames = plugin->SupportedInterfaces();
          if (interfaceNames.empty()) {
            // This plugin does not support any interfaces, skip it
            break;
          }
          // Create a new registry endpoint with the same endpoint and a specified interface
          RegistryEndpoint registry = a->registry;
          // We will use the first interfaceName this plugin supports
          registry.InterfaceName = interfaceNames.front();
          logger.msg(DEBUG, "New registry endpoint is created from the one with the unspecified interface: " + registry.str());
          newRegistries.push_back(registry);
          // Create a new thread argument (by copying the original one) for the sub-thread which will query this interface
          ThreadArgSER* newArg = new ThreadArgSER(*a);
          newArg->registry = registry;
          newArg->pluginName = *it;
          // The new thread counter contians a new ThreadedPointer pointing to the main threadCounter,
          // but want it to point to our new subthreadCounter instead
          newArg->threadCounter = subthreadCounter;
          // The sub-threads have to know that they are sub-threads,
          // and that they have to set the counter to 0 in case of a successful query
          newArg->subthread = true;
          logger.msg(DEBUG, "Starting sub-thread to query the registry on " + registry.str());
          /* modified lock */ serCommon->mutex.lockShared(); if (!serCommon->isActive) { serCommon->mutex.unlockShared(); a->threadCounter->dec(); delete a; delete newArg; return; }
          subthreadCounter->inc();
          if (!CreateThreadFunction(&queryRegistry, newArg)) {
            logger.msg(ERROR, "Failed to start querying the registry on " + registry.str() + " (unable to create sub-thread)");
            subthreadCounter->dec();
            delete newArg;
          }
          /* unlock */  serCommon->mutex.unlockShared();
        }
        // We wait until the counter is set to (or below) zero, which can happen in two cases
        //   1. one sub-thread was succesful
        //   2. all the sub-threads failed
        subthreadCounter->wait();
        // Check which case happened
        /* lock */ serCommon->mutex.lockShared(); if (!serCommon->isActive) { serCommon->mutex.unlockShared(); delete a; a->threadCounter->dec(); return; }
        size_t failedCount = 0;
        bool wasSuccesful = false;
        for (std::list<RegistryEndpoint>::iterator it = newRegistries.begin(); it != newRegistries.end(); it++) {
          RegistryEndpointStatus status = a->ser->getStatusOfRegistry(*it);
          if (status.status == SER_SUCCESSFUL) {
            wasSuccesful = true;
            break;
          } else if (status.status == SER_FAILED) {
            failedCount++;
          }
        }
        // Set the status of the original registry (the one without the specified interface)
        if (wasSuccesful) {
          a->ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_SUCCESSFUL));
        } else if (failedCount == newRegistries.size()) {
          a->ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_FAILED));
        } else {
          a->ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_UNKNOWN));
        }
        /* unlock */ serCommon->mutex.unlockShared();
      }
    }
    a->threadCounter->dec();
    delete a;
  }




  // TESTControl

  float ServiceEndpointRetrieverTESTControl::delay = 0;
  RegistryEndpointStatus ServiceEndpointRetrieverTESTControl::status;
  std::list<ServiceEndpoint> ServiceEndpointRetrieverTESTControl::endpoints = std::list<ServiceEndpoint>();
}
