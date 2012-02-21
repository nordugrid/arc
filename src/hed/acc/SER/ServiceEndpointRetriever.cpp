// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/client/EndpointQueryingStatus.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

  Logger ServiceEndpointRetriever::logger(Logger::getRootLogger(), "ServiceEndpointRetriever");

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
    std::list<std::string> types(serCommon->getListOfPlugins());
    // Map supported interfaces to available plugins.
    for (std::list<std::string>::const_iterator itT = types.begin(); itT != types.end(); ++itT) {
      ServiceEndpointRetrieverPlugin* p = serCommon->load(*itT);
      if (p) {
        for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
          // If two plugins supports two identical interfaces, then only the last will appear in the map.
          interfacePluginMap[*itI] = *itT;
        }
      }
    }
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
        logger.msg(DEBUG, "Unable to find ServiceEndpointRetrieverPlugin plugin to query interface \"%s\" on \"%s\"", registry.InterfaceName, registry.Endpoint);
        setStatusOfRegistry(registry, EndpointQueryingStatus(EndpointQueryingStatus::NOPLUGIN));
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
      setStatusOfRegistry(registry, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
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

  EndpointQueryingStatus ServiceEndpointRetriever::getStatusOfRegistry(RegistryEndpoint registry) const {
    statusLock.lock();
    EndpointQueryingStatus status(EndpointQueryingStatus::UNKNOWN);
    std::map<RegistryEndpoint, EndpointQueryingStatus>::const_iterator it = statuses.find(registry);
    if (it != statuses.end()) {
      status = it->second;
    }
    statusLock.unlock();
    return status;
  }

  bool ServiceEndpointRetriever::setStatusOfRegistry(const RegistryEndpoint& registry, const EndpointQueryingStatus& status, bool overwrite) {
    statusLock.lock();
    bool wasSet = false;
    if (overwrite || (statuses.find(registry) == statuses.end())) {
      logger.msg(DEBUG, "Setting status (%s) for registry: %s", status.str(), registry.str());
      statuses[registry] = status;
      wasSet = true;
    }
    statusLock.unlock();
    return wasSet;
  };

  void ServiceEndpointRetriever::queryRegistry(void *arg) {
    AutoPointer<ThreadArgSER> a((ThreadArgSER*)arg);
    ThreadedPointer<SERCommon>& serCommon = a->serCommon;
    bool set = false;
    // Set the status of the registry to STARTED only if it was not set already by an other thread (overwrite = false)
    if(!serCommon->lockSharedIfValid()) return;
    set = (*serCommon)->setStatusOfRegistry(a->registry, EndpointQueryingStatus(EndpointQueryingStatus::STARTED), false);
    serCommon->unlockShared();

    if (!set) { // The thread was not able to set the status (because it was already set by another thread)
      logger.msg(DEBUG, "Will not query registry (%s) because another thread is already querying it", a->registry.str());
      return;
    }
    // If the thread was able to set the status, then this is the first (and only) thread querying this registry
    if (!a->pluginName.empty()) { // If the plugin was already selected
      ServiceEndpointRetrieverPlugin* plugin = serCommon->load(a->pluginName);
      if (!plugin) {
        if(!serCommon->lockSharedIfValid()) return;
        (*serCommon)->setStatusOfRegistry(a->registry, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
        serCommon->unlockShared();
        return;
      }
      logger.msg(DEBUG, "Calling plugin %s to query registry on %s", a->pluginName, a->registry.str());
      std::list<ServiceEndpoint> endpoints;
      // Do the actual querying against service.
      EndpointQueryingStatus status = plugin->Query(*serCommon, a->registry, endpoints, a->capabilityFilter);
      for (std::list<ServiceEndpoint>::iterator it = endpoints.begin(); it != endpoints.end(); it++) {
        if(!serCommon->lockSharedIfValid()) return;
        (*serCommon)->addServiceEndpoint(*it);
        serCommon->unlockShared();
      }

      if(!serCommon->lockSharedIfValid()) return;
      (*serCommon)->setStatusOfRegistry(a->registry, status);
      serCommon->unlockShared();
      if (status) a->serResult.setSuccess(); // Successful query
    } else { // If there was no plugin selected for this registry, this will try all possibility
      logger.msg(DEBUG, "The interface of this registry endpoint (%s) is unspecified, will try all possible plugins", a->registry.str());
      std::list<std::string> types = serCommon->getListOfPlugins();
      // A list for collecting the new registry endpoints which will be created by copying the original one
      // and setting the InterfaceName for each possible plugins
      std::list<RegistryEndpoint> newRegistries;
      // A new result object is created for the sub-threads, "true" means we only want to wait for the first successful query
      SERResult newserResult(true);
      for (std::list<std::string>::const_iterator it = types.begin(); it != types.end(); ++it) {
        ServiceEndpointRetrieverPlugin* plugin = serCommon->load(*it);
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
        // Create a new registry endpoint with the same endpoint and a specified interface
        RegistryEndpoint registry = a->registry;
        // We will use the first interfaceName this plugin supports
        registry.InterfaceName = interfaceNames.front();
        logger.msg(DEBUG, "New registry endpoint is created (%s) from the one with the unspecified interface (%s)", registry.str(), a->registry.str());
        newRegistries.push_back(registry);
        // Make new argument by copying old one with result report object replaced
        ThreadArgSER* newArg = new ThreadArgSER(*a,newserResult);
        newArg->registry = registry;
        newArg->pluginName = *it;
        logger.msg(DEBUG, "Starting sub-thread to query the registry on %s", registry.str());
        if (!CreateThreadFunction(&queryRegistry, newArg)) {
          logger.msg(ERROR, "Failed to start querying the registry on %s (unable to create sub-thread)", registry.str());
          delete newArg;
          if(!serCommon->lockSharedIfValid()) return;
          (*serCommon)->setStatusOfRegistry(registry, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
          serCommon->unlockShared();
          continue;
        }
      }
      // We wait for the new result object. The wait returns in two cases:
      //   1. one sub-thread was succesful
      //   2. all the sub-threads failed
      newserResult.wait();
      // Check which case happened
      if(!serCommon->lockSharedIfValid()) return;
      size_t failedCount = 0;
      bool wasSuccesful = false;
      for (std::list<RegistryEndpoint>::iterator it = newRegistries.begin(); it != newRegistries.end(); it++) {
        EndpointQueryingStatus status = (*serCommon)->getStatusOfRegistry(*it);
        if (status) {
          wasSuccesful = true;
          break;
        } else if (status == EndpointQueryingStatus::FAILED) {
          failedCount++;
        }
      }
      // Set the status of the original registry (the one without the specified interface)
      if (wasSuccesful) {
        (*serCommon)->setStatusOfRegistry(a->registry, EndpointQueryingStatus(EndpointQueryingStatus::SUCCESSFUL));
      } else if (failedCount == newRegistries.size()) {
        (*serCommon)->setStatusOfRegistry(a->registry, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
      } else {
        (*serCommon)->setStatusOfRegistry(a->registry, EndpointQueryingStatus(EndpointQueryingStatus::UNKNOWN));
      }
      serCommon->unlockShared();
    }
  }




  // TESTControl

  float ServiceEndpointRetrieverPluginTESTControl::delay = 0;
  EndpointQueryingStatus ServiceEndpointRetrieverPluginTESTControl::status;
  std::list<ServiceEndpoint> ServiceEndpointRetrieverPluginTESTControl::endpoints = std::list<ServiceEndpoint>();
}
