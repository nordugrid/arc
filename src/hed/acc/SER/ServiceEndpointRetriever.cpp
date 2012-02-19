// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/Utils.h>

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
  
  const std::string RegistryEndpoint::RegistryCapability = "information.discovery.registry";
  

  ServiceEndpointRetriever::ServiceEndpointRetriever(const UserConfig& uc,
                                                     bool recursive,
                                                     std::list<std::string> capabilityFilter)
    : serCommon(new SERCommon(this,uc)),
      serResult(),
      uc(uc),
      recursive(recursive),
      capabilityFilter(capabilityFilter)
  {
    // Used for holding names of all available plugins.
    std::list<std::string> types(serCommon->Loader().getListOfPlugins());
    // Map supported interfaces to available plugins.
    for (std::list<std::string>::const_iterator itT = types.begin(); itT != types.end(); ++itT) {
      ServiceEndpointRetrieverPlugin* p = serCommon->Loader().load(*itT);
      if (p) {
        for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
          // If two plugins supports two identical interfaces, then only the last will appear in the map.
          interfacePluginMap[*itI] = *itT;
        }
      }
    }
  }

  ServiceEndpointRetriever::~ServiceEndpointRetriever() {
    serCommon->Deactivate();
  }

  void ServiceEndpointRetriever::addConsumer(ServiceEndpointConsumer& consumer) {
    consumerLock.lock();
    consumers.push_back(&consumer);
    consumerLock.unlock();  
  }
  
  void ServiceEndpointRetriever::removeConsumer(const ServiceEndpointConsumer& consumer) {
    consumerLock.lock();
    std::list<ServiceEndpointConsumer*>::iterator it = std::find(consumers.begin(), consumers.end(), &consumer);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
    consumerLock.unlock();
  }

  void ServiceEndpointRetriever::addRegistryEndpoint(const RegistryEndpoint& registry) {
    std::map<std::string, std::string>::const_iterator itPluginName = interfacePluginMap.end();
    if (!registry.InterfaceName.empty()) {
      itPluginName = interfacePluginMap.find(registry.InterfaceName);
      if (itPluginName == interfacePluginMap.end()) {
        logger.msg(DEBUG, "Unable to find ServiceRetrieverPlugin plugin to query interface \"%s\" on \"%s\"", registry.InterfaceName, registry.Endpoint);
        setStatusOfRegistry(registry, RegistryEndpointStatus(SER_NOPLUGIN));
        return;
      }
    }
    // serCommon will be copied into the thread arg,
    // which means that all threads will have a new 
    // instance of the ThreadedPointer pointing to the same object
    ThreadArgSER *arg = new ThreadArgSER(serCommon, serResult);
    arg->registry = registry;
    arg->capabilityFilter = capabilityFilter;
    // For recursivity the plugins should return registries even if they would be filtered
    if (recursive) {
      arg->capabilityFilter.push_back(RegistryEndpoint::RegistryCapability);
    }
    if (itPluginName != interfacePluginMap.end()) {
      arg->pluginName = itPluginName->second;
    }
    logger.msg(Arc::DEBUG, "Starting thread to query the registry on %s", arg->registry.str());
    if (!CreateThreadFunction(&queryRegistry, arg)) {
      logger.msg(Arc::ERROR, "Failed to start querying the registry on %s", arg->registry.str() + " (unable to create thread)");
      setStatusOfRegistry(registry, RegistryEndpointStatus(SER_FAILED));
      delete arg;
    }
  }

  void ServiceEndpointRetriever::addServiceEndpoint(const ServiceEndpoint& service) {
    if (recursive && RegistryEndpoint::isRegistry(service)) {
      RegistryEndpoint registry(service);
      logger.msg(Arc::DEBUG, "Found a registry, will query it recursively: %s", registry.str());
      addRegistryEndpoint(registry);
    }

    bool match = false;
    for (std::list<std::string>::iterator it = capabilityFilter.begin(); it != capabilityFilter.end(); it++) {
      if (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), *it) != service.EndpointCapabilities.end()) {
        match = true;
        break;
      }
    }
    if (capabilityFilter.empty() || match) {
      consumerLock.lock();
      for (std::list<ServiceEndpointConsumer*>::iterator it = consumers.begin(); it != consumers.end(); it++) {
        (*it)->addServiceEndpoint(service);        
      }
      consumerLock.unlock();
    }
  }

  void ServiceEndpointRetriever::wait() const {
    serResult.Wait();
  };

  bool ServiceEndpointRetriever::isDone() const {
    return serResult.Wait(0);
  };

  RegistryEndpointStatus ServiceEndpointRetriever::getStatusOfRegistry(RegistryEndpoint registry) const {
    statusLock.lock();
    RegistryEndpointStatus status(SER_UNKNOWN);
    std::map<RegistryEndpoint, RegistryEndpointStatus>::const_iterator it = statuses.find(registry);
    if (it != statuses.end()) {
      status = it->second;
    }
    statusLock.unlock();
    return status;
  }

  bool ServiceEndpointRetriever::setStatusOfRegistry(const RegistryEndpoint& registry, const RegistryEndpointStatus& status, bool overwrite) {
    statusLock.lock();
    bool wasSet = false;
    if (overwrite || (statuses.find(registry) == statuses.end())) {
      logger.msg(DEBUG, "Setting status (%s) for registry: %s", string(status.status), registry.str());
      statuses[registry] = status;
      wasSet = true;
    }
    statusLock.unlock();
    return wasSet;
  };

  void ServiceEndpointRetriever::queryRegistry(void *arg) {
    AutoPointer<ThreadArgSER> a((ThreadArgSER*)arg);
    ThreadedPointer<SERCommon>& serCommon = a->serCommon;
    ServiceEndpointRetriever* ser = NULL;
    bool set = false;
    // Set the status of the registry to STARTED only if it was not set already by an other thread (overwrite = false)
    if(!(ser = serCommon->lockShared())) return;
    set = ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_STARTED), false);
    serCommon->unlockShared();
    
    if (!set) { // The thread was not able to set the status (because it was already set by another thread)
      logger.msg(DEBUG, "Will not query registry (%s) because another thread is already querying it", a->registry.str());
      return;
    }
    // If the thread was able to set the status, then this is the first (and only) thread querying this registry
    if (!a->pluginName.empty()) { // If the plugin was already selected
      ServiceEndpointRetrieverPlugin* plugin = serCommon->Loader().load(a->pluginName);
      if (!plugin) {
        if(!(ser = serCommon->lockShared())) return;
        ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_FAILED));
        serCommon->unlockShared();
        return;
      }
      logger.msg(DEBUG, "Calling plugin %s to query registry on %s", a->pluginName, a->registry.str());
      std::list<ServiceEndpoint> endpoints;
      // Do the actual querying against service.
      RegistryEndpointStatus status = plugin->Query(serCommon->Cfg(), a->registry, endpoints, a->capabilityFilter);
      for (std::list<ServiceEndpoint>::iterator it = endpoints.begin(); it != endpoints.end(); it++) {
        if(!(ser = serCommon->lockShared())) return;
        ser->addServiceEndpoint(*it);
        serCommon->unlockShared();
      }

      if(!(ser = serCommon->lockShared())) return;
      ser->setStatusOfRegistry(a->registry, status);
      serCommon->unlockShared();
      if (status.status == SER_SUCCESSFUL) a->serResult.Success(); // Successful query
    } else { // If there was no plugin selected for this registry, this will try all possibility
      logger.msg(DEBUG, "The interface of this registry endpoint (%s) is unspecified, will try all possible plugins", a->registry.str());
      std::list<std::string> types = serCommon->Loader().getListOfPlugins();
      // A list for collecting the new registry endpoints which will be created by copying the original one
      // and setting the InterfaceName for each possible plugins
      std::list<RegistryEndpoint> newRegistries;
      // A new counter is needed for the subthreads
      SERResult(true);
      for (std::list<std::string>::const_iterator it = types.begin(); it != types.end(); ++it) {
        ServiceEndpointRetrieverPlugin* plugin = serCommon->Loader().load(*it);
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
        logger.msg(DEBUG, "New registry endpoint is created (%s) from the one with the unspecified interface (%s)", registry.str(), a->registry.str());
        newRegistries.push_back(registry);
        // Create a new thread argument (by copying the original one) for the sub-thread which will query this interface

        // The sub-threads have to know that they are sub-threads,
        // and that they have to set the counter to 0 in case of a successful query
        SERResult newserResult(true);
        // Make new argument by copying old one with result report object replaced
        ThreadArgSER* newArg = new ThreadArgSER(*a,newserResult);
        newArg->registry = registry;
        newArg->pluginName = *it;
        logger.msg(DEBUG, "Starting sub-thread to query the registry on %s", registry.str());
        if (!CreateThreadFunction(&queryRegistry, newArg)) {
          logger.msg(ERROR, "Failed to start querying the registry on %s (unable to create sub-thread)", registry.str());
          delete newArg;
          return;
        }
        // We wait until the counter is set to (or below) zero, which can happen in two cases
        //   1. one sub-thread was succesful
        //   2. all the sub-threads failed
        newserResult.Wait();
        // Check which case happened
        if(!(ser = serCommon->lockShared())) return;
        size_t failedCount = 0;
        bool wasSuccesful = false;
        for (std::list<RegistryEndpoint>::iterator it = newRegistries.begin(); it != newRegistries.end(); it++) {
          RegistryEndpointStatus status = ser->getStatusOfRegistry(*it);
          if (status.status == SER_SUCCESSFUL) {
            wasSuccesful = true;
            break;
          } else if (status.status == SER_FAILED) {
            failedCount++;
          }
        }
        // Set the status of the original registry (the one without the specified interface)
        if (wasSuccesful) {
          ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_SUCCESSFUL));
        } else if (failedCount == newRegistries.size()) {
          ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_FAILED));
        } else {
          ser->setStatusOfRegistry(a->registry, RegistryEndpointStatus(SER_UNKNOWN));
        }
        serCommon->unlockShared();
      } // for (types)
    }
  }




  // TESTControl

  float ServiceEndpointRetrieverTESTControl::delay = 0;
  RegistryEndpointStatus ServiceEndpointRetrieverTESTControl::status;
  std::list<ServiceEndpoint> ServiceEndpointRetrieverTESTControl::endpoints = std::list<ServiceEndpoint>();
}
