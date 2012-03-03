// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/Job.h>

#include "EndpointRetriever.h"

namespace Arc {

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
    logger.msg(DEBUG, "Loaded %s %s", EndpointRetrieverPlugin<T, S>::kind, name);
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
  EndpointRetriever<T, S>::EndpointRetriever(const UserConfig& uc, const EndpointQueryOptions<S>& options)
    : common(new Common(this, uc)),
      result(),
      uc(uc),
      options(options)
  {
    // Used for holding names of all available plugins.
    std::list<std::string> availablePlugins = common->getListOfPlugins();

    // Map supported interfaces to available plugins.
    for (std::list<std::string>::iterator itT = availablePlugins.begin(); itT != availablePlugins.end();) {
      EndpointRetrieverPlugin<T, S>* p = common->load(*itT);

      if (!p) {
        itT = availablePlugins.erase(itT);
        continue;
      }

      const std::list<std::string>& interfaceNames = p->SupportedInterfaces();

      if (interfaceNames.empty()) {
        // This plugin does not support any interfaces, skip it
        logger.msg(DEBUG, "The plugin %s does not support any interfaces, skipping it.", *itT);
        itT = availablePlugins.erase(itT);
        continue;
      }
      else if (interfaceNames.front().empty()) {
        logger.msg(DEBUG, "The first supported interface of the plugin %s is an empty string, skipping the plugin.", *itT);
        itT = availablePlugins.erase(itT);
        continue;
      }

      for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
        // If two plugins supports two identical interfaces, then only the last will appear in the map.
        interfacePluginMap[*itI] = *itT;
      }

      ++itT;
    }
    
    common->setAvailablePlugins(availablePlugins);
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
        //logger.msg(DEBUG, "Unable to find TargetInformationRetrieverPlugin plugin to query interface \"%s\" on \"%s\"", endpoint.InterfaceName, endpoint.URLString);
        setStatusOfEndpoint(endpoint, EndpointQueryingStatus(EndpointQueryingStatus::NOPLUGIN));
        return;
      }
    }
    // common will be copied into the thread arg,
    // which means that all threads will have a new
    // instance of the ThreadedPointer pointing to the same object
    ThreadArg *arg = new ThreadArg(common, result, endpoint, options);
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

  template<>
  void EndpointRetriever<RegistryEndpoint, ServiceEndpoint>::addEndpoint(const ServiceEndpoint& endpoint) {
    // Check if the service is among the rejected ones
    const std::list<std::string>& rejectedServices = options.getRejectedServices();
    URL url(endpoint.URLString);
    for (std::list<std::string>::const_iterator it = rejectedServices.begin(); it != rejectedServices.end(); it++) {
      if (url.StringMatches(*it)) {
        return;
      }
    }
    if (options.recursiveEnabled() && RegistryEndpoint::isRegistry(endpoint)) {
      RegistryEndpoint registry(endpoint);
      logger.msg(DEBUG, "Found a registry, will query it recursively: %s", registry.str());
      EndpointRetriever<RegistryEndpoint, ServiceEndpoint>::addEndpoint(registry);
    }

    bool match = false;
    for (std::list<std::string>::const_iterator it = options.getCapabilityFilter().begin(); it != options.getCapabilityFilter().end(); it++) {
      if (std::find(endpoint.Capability.begin(), endpoint.Capability.end(), *it) != endpoint.Capability.end()) {
        match = true;
        break;
      }
    }
    if (options.getCapabilityFilter().empty() || match) {
      consumerLock.lock();
      for (std::list<EndpointConsumer<ServiceEndpoint>*>::iterator it = consumers.begin(); it != consumers.end(); it++) {
        (*it)->addEndpoint(endpoint);
      }
      consumerLock.unlock();
    }
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
      EndpointQueryingStatus status = plugin->Query(*common, a->endpoint, endpoints, a->options);
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
      const std::list<std::string>& preferredInterfaceNames = a->options.getPreferredInterfaceNames();
      // A list for collecting the new endpoints which will be created by copying the original one
      // and setting the InterfaceName for each possible plugins
      std::list<T> preferredEndpoints;
      std::list<T> otherEndpoints;
      // A new result object is created for the sub-threads, "true" means we only want to wait for the first successful query
      Result preferredResult(true);
      Result otherResult(true);
      for (std::list<std::string>::const_iterator it = common->getAvailablePlugins().begin(); it != common->getAvailablePlugins().end(); ++it) {
        EndpointRetrieverPlugin<T, S>* plugin = common->load(*it);
        if (!plugin) {
          // Should not happen since all available plugins was already loaded in the constructor.
          // Problem loading the plugin, skip it
          logger.msg(DEBUG, "Problem loading plugin %s, skipping it..", *it);
          continue;
        }
        if (plugin->isEndpointNotSupported(a->endpoint)) {
          logger.msg(DEBUG, "The endpoint (%s) is not supported by this plugin (%s)", a->endpoint.URLString, *it);
          continue;
        }
        // Create a new endpoint with the same endpoint and a specified interface
        T endpoint = a->endpoint;
        ThreadArg* newArg; 
        // Set interface
        std::list<std::string>::const_iterator itSI = plugin->SupportedInterfaces().begin();
        for (; itSI != plugin->SupportedInterfaces().end(); ++itSI) {
          if (std::find(preferredInterfaceNames.begin(), preferredInterfaceNames.end(), *itSI) != preferredInterfaceNames.end()) {
            endpoint.InterfaceName = *itSI; // TODO: *itSI must not be empty.
            preferredEndpoints.push_back(endpoint);
            newArg = new ThreadArg(*a, preferredResult);
            break;
          }
        }

        if (itSI == plugin->SupportedInterfaces().end()) {
          // We will use the first interfaceName this plugin supports
          endpoint.InterfaceName = plugin->SupportedInterfaces().front();
          logger.msg(DEBUG, "New endpoint is created (%s) from the one with the unspecified interface (%s)",  endpoint.str(), a->endpoint.str());
          otherEndpoints.push_back(endpoint);
          newArg = new ThreadArg(*a, otherResult);
        }
        // Make new argument by copying old one with result report object replaced
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

      // We wait for the preferred result object. The wait returns in two cases:
      //   1. one sub-thread was succesful
      //   2. all the sub-threads failed
      preferredResult.wait();
      // Check which case happened
      if(!common->lockSharedIfValid()) return;
      EndpointQueryingStatus status;
      for (typename std::list<T>::const_iterator it = preferredEndpoints.begin(); it != preferredEndpoints.end(); it++) {
        status = (*common)->getStatusOfEndpoint(*it);
        if (status) {
          break;
        }
      }
      // Set the status of the original endpoint (the one without the specified interface)
      if (!status) {
        // Wait for the other threads, maybe they were successful
        otherResult.wait();
        typename std::list<T>::const_iterator it = otherEndpoints.begin();
        for (; it != otherEndpoints.end(); ++it) {
          status = (*common)->getStatusOfEndpoint(*it);
          if (status) {
            break;
          }
        }
        if (it == otherEndpoints.end()) {
          /* TODO: In case of failure of all plugins, a clever and
           * helpful message should be set. Maybe an algorithm for
           * picking the most suitable failure message among the used
           * plugins.
           */
          status = EndpointQueryingStatus(EndpointQueryingStatus::FAILED);
        }
      }
      
      (*common)->setStatusOfEndpoint(a->endpoint, status);
      common->unlockShared();
    }
  }

  ExecutionTargetRetriever::ExecutionTargetRetriever(
    const UserConfig& uc,
    const std::list<ServiceEndpoint>& services,
    const std::list<std::string>& rejectedServices,
    const std::list<std::string>& preferredInterfaceNames,
    const std::list<std::string>& capabilityFilter
  ) : ser(uc, EndpointQueryOptions<ServiceEndpoint>(true, capabilityFilter, rejectedServices)),
      tir(uc, EndpointQueryOptions<ExecutionTarget>(preferredInterfaceNames))
  {
    ser.addConsumer(*this);
    tir.addConsumer(*this);
    for (std::list<ServiceEndpoint>::const_iterator it = services.begin(); it != services.end(); it++) {
      if (RegistryEndpoint::isRegistry(*it)) {
        ser.addEndpoint(RegistryEndpoint(*it));
      } else {
        // our own addEndpoint, which will send it to the TIR
        addEndpoint(*it);
      }
    }
  }
  
  void ExecutionTargetRetriever::addEndpoint(const ServiceEndpoint& service) {
    // If we got a computing element info endpoint, then we pass it to the TIR
    if (ComputingInfoEndpoint::isComputingInfo(service)) {
      tir.addEndpoint(ComputingInfoEndpoint(service));
    }
  }
  
  template class EndpointRetriever<RegistryEndpoint, ServiceEndpoint>;
  template class EndpointRetrieverPlugin<RegistryEndpoint, ServiceEndpoint>;
  template class EndpointRetrieverPluginLoader<RegistryEndpoint, ServiceEndpoint>;
  template<> Logger EndpointRetriever<RegistryEndpoint, ServiceEndpoint>::logger(Logger::getRootLogger(), "ServiceEndpointRetriever");
  template<> const std::string EndpointRetrieverPlugin<RegistryEndpoint, ServiceEndpoint>::kind("HED:ServiceEndpointRetrieverPlugin");
  template<> Logger EndpointRetrieverPluginLoader<RegistryEndpoint, ServiceEndpoint>::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPluginLoader");

  template class EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget>;
  template class EndpointRetrieverPlugin<ComputingInfoEndpoint, ExecutionTarget>;
  template class EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget>;
  template<> Logger EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget>::logger(Logger::getRootLogger(), "TargetInformationRetriever");
  template<> Logger EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget>::logger(Logger::getRootLogger(), "TargetInformationRetrieverPluginLoader");
  template<> const std::string TargetInformationRetrieverPlugin::kind("HED:TargetInformationRetrieverPlugin");

  template class EndpointRetriever<ComputingInfoEndpoint, Job>;
  template class EndpointRetrieverPlugin<ComputingInfoEndpoint, Job>;
  template class EndpointRetrieverPluginLoader<ComputingInfoEndpoint, Job>;
  template<> Logger EndpointRetriever<ComputingInfoEndpoint, Job>::logger(Logger::getRootLogger(), "JobListRetriever");
  template<> const std::string EndpointRetrieverPlugin<ComputingInfoEndpoint, Job>::kind("HED:JobListRetrieverPlugin");
  template<> Logger EndpointRetrieverPluginLoader<ComputingInfoEndpoint, Job>::logger(Logger::getRootLogger(), "JobListRetrieverPluginLoader");


} // namespace Arc
