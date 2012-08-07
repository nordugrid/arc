// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/Job.h>

#include "EntityRetriever.h"

namespace Arc {

  template<typename T>
  EntityRetrieverPluginLoader<T>::~EntityRetrieverPluginLoader() {
    for (typename std::map<std::string, EntityRetrieverPlugin<T> *>::iterator it = plugins.begin();
         it != plugins.end(); it++) {
      delete it->second;
    }
  }

  template<typename T>
  EntityRetrieverPlugin<T>* EntityRetrieverPluginLoader<T>::load(const std::string& name) {
    if (plugins.find(name) != plugins.end()) {
      logger.msg(DEBUG, "Found %s %s (it was loaded already)", EntityRetrieverPlugin<T>::kind, name);
      return plugins[name];
    }

    if (name.empty()) {
      return NULL;
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(), EntityRetrieverPlugin<T>::kind, name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for %s plugin is installed", name, name);
      logger.msg(DEBUG, "%s plugin \"%s\" not found.", EntityRetrieverPlugin<T>::kind, name, name);
      return NULL;
    }

     EntityRetrieverPlugin<T> *p = factory_->GetInstance< EntityRetrieverPlugin<T> >(EntityRetrieverPlugin<T>::kind, name, NULL, false);

    if (!p) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "%s %s could not be created.", EntityRetrieverPlugin<T>::kind, name, name);
      return NULL;
    }

    plugins[name] = p;
    logger.msg(DEBUG, "Loaded %s %s", EntityRetrieverPlugin<T>::kind, name);
    return p;
  }

  template<typename T>
  std::list<std::string> EntityRetrieverPluginLoader<T>::getListOfPlugins() {
    std::list<ModuleDesc> modules;
    PluginsFactory factory(BaseConfig().MakeConfig(Config()).Parent());
    factory.scan(FinderLoader::GetLibrariesList(), modules);
    PluginsFactory::FilterByKind(EntityRetrieverPlugin<T>::kind, modules);
    std::list<std::string> names;
    for (std::list<ModuleDesc>::const_iterator it = modules.begin(); it != modules.end(); it++) {
      for (std::list<PluginDesc>::const_iterator it2 = it->plugins.begin(); it2 != it->plugins.end(); it2++) {
        names.push_back(it2->name);
      }
    }
    return names;
  }

  template<typename T>
  EntityRetriever<T>::EntityRetriever(const UserConfig& uc, const EndpointQueryOptions<T>& options)
    : common(new Common(this, uc)),
      result(),
      uc(uc),
      options(options)
  {
    // Used for holding names of all available plugins.
    std::list<std::string> availablePlugins = common->getListOfPlugins();

    // Map supported interfaces to available plugins.
    for (std::list<std::string>::iterator itT = availablePlugins.begin(); itT != availablePlugins.end();) {
      EntityRetrieverPlugin<T>* p = common->load(*itT);

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

  template<typename T>
  void EntityRetriever<T>::removeConsumer(const EntityConsumer<T>& consumer) {
    consumerLock.lock();
    typename std::list< EntityConsumer<T>* >::iterator it = std::find(consumers.begin(), consumers.end(), &consumer);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
    consumerLock.unlock();
  }

  template<typename T>
  void EntityRetriever<T>::addEndpoint(const Endpoint& endpoint) {
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

  template<typename T>
  void EntityRetriever<T>::addEntity(const T& entity) {
    consumerLock.lock();
    for (typename std::list< EntityConsumer<T>* >::iterator it = consumers.begin(); it != consumers.end(); it++) {
      (*it)->addEntity(entity);
    }
    consumerLock.unlock();
  }

  template<>
  void EntityRetriever<Endpoint>::addEntity(const Endpoint& endpoint) {
    // Check if the service is among the rejected ones
    const std::list<std::string>& rejectedServices = options.getRejectedServices();
    URL url(endpoint.URLString);
    for (std::list<std::string>::const_iterator it = rejectedServices.begin(); it != rejectedServices.end(); it++) {
      if (url.StringMatches(*it)) {
        return;
      }
    }
    if (options.recursiveEnabled() && endpoint.HasCapability(Endpoint::REGISTRY)) {
      Endpoint registry(endpoint);
      logger.msg(DEBUG, "Found a registry, will query it recursively: %s", registry.str());
      EntityRetriever<Endpoint>::addEndpoint(registry);
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
      for (std::list<EntityConsumer<Endpoint>*>::iterator it = consumers.begin(); it != consumers.end(); it++) {
        (*it)->addEntity(endpoint);
      }
      consumerLock.unlock();
    }
  }

  template<typename T>
  EndpointQueryingStatus EntityRetriever<T>::getStatusOfEndpoint(const Endpoint& endpoint) const {
    statusLock.lock();
    EndpointQueryingStatus status(EndpointQueryingStatus::UNKNOWN);
    typename std::map<Endpoint, EndpointQueryingStatus>::const_iterator it = statuses.find(endpoint);
    if (it != statuses.end()) {
      status = it->second;
    }
    statusLock.unlock();
    return status;
  }

  template<typename T>
  bool EntityRetriever<T>::setStatusOfEndpoint(const Endpoint& endpoint, const EndpointQueryingStatus& status, bool overwrite) {
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

  template<typename T>
  void EntityRetriever<T>::getServicesWithStatus(const EndpointQueryingStatus& status, std::set<std::string>& result) {
    statusLock.lock();
    for (std::map<Endpoint, EndpointQueryingStatus>::const_iterator it = statuses.begin();
         it != statuses.end(); ++it) {
      if (it->second == status) result.insert(it->first.getServiceName());
    }
    statusLock.unlock();
  }


  /* Overview of how the queryEndpoint algorithm works
   * The queryEndpoint method is meant to be run in a separate thread, spawned
   * by the addEndpoint method. Furthermore it is designed to call it self, when
   * the Endpoint has no interface specified, in order to check all supported
   * interfaces. Since the method is static, common data is reached through a
   * Common class object, wrapped and protected by the ThreadedPointer template
   * class.
   * 
   * Taking as starting point when the addEndpoint method calls the
   * queryEndpoint method these are the steps the queryEndpoint method goes
   * through:
   * 1. Checks whether the Endpoint has already been queried, or is in the
   *    process of being queried. If that is the case, it
   *    stops. Otherwise branch a is followed if the Endpoint has a interface
   *    specified, if not then branch b is followed.
   * 2a. The plugin corresponding to the chosen interface is loaded and its
   *     Query method is called passing a container used to store the result of
   *     query. This call blocks. 
   * 3a. When the Query call finishes the container is iterated, and each item
   *     in the container is passed on to the EntityRetriever::addEntity method,
   *     then status returned from the Query method is registered with the
   *     EntityRetriever object through the setStatusOfEndpoint method, and if
   *     status is successful the common Result object is set to be successful.
   *     Then the method returns.
   * 2b. The list of available plugins is looped, and plugins are loaded. If
   *     they fail loading the loop continues. Then it is determined whether the
   *     specific plugin supports a preferred interface as specified in the
   *     EndpointQueryOptions object, or not. In the end of the loop a new
   *     thread is spawned calling this method itself, but with an Endpoint with
   *     specified interface, thus in that call going through branch a.
   * 3b. After the loop, the wait method is called on the preferred Result
   *     object, thus waiting for the querying of the preferred interfaces to
   *     succeed. If any of these succeeded successfully then the status of the
   *     Endpoint object with unspecified interface in status-map is set
   *     accordingly. If not then the wait method on the other Result object is
   *     called, and a similiar check and set is one. Then the method returns.
   */
  template<typename T>
  void EntityRetriever<T>::queryEndpoint(void *arg) {
    AutoPointer<ThreadArg> a((ThreadArg*)arg);
    ThreadedPointer<Common>& common = a->common;

    // Set the status of the endpoint to STARTED only if it was not set already by an other thread (overwrite = false)
    bool set = false;
    if(!common->lockSharedIfValid()) return;
    set = (*common)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::STARTED), false);
    common->unlockShared();
    if (!set) { // The thread was not able to set the status (because it was already set by another thread)
      logger.msg(DEBUG, "Will not query endpoint (%s) because another thread is already querying it", a->endpoint.str());
      return;
    }
    
    // If the thread was able to set the status, then this is the first (and only) thread querying this endpoint
    if (!a->pluginName.empty()) { // If the plugin was already selected
      EntityRetrieverPlugin<T>* plugin = common->load(a->pluginName);
      if (!plugin) {
        if(!common->lockSharedIfValid()) return;
        (*common)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
        common->unlockShared();
        return;
      }
      logger.msg(DEBUG, "Calling plugin %s to query endpoint on %s", a->pluginName, a->endpoint.str());

      // Do the actual querying against service.
      std::list<T> entities;
      EndpointQueryingStatus status = plugin->Query(*common, a->endpoint, entities, a->options);

      // Add obtained results from querying to registered consumers (addEntity method)
      for (typename std::list<T>::const_iterator it = entities.begin(); it != entities.end(); ++it) {
        if(!common->lockSharedIfValid()) return;
        (*common)->addEntity(*it);
        common->unlockShared();
      }

      if(!common->lockSharedIfValid()) return;
      (*common)->setStatusOfEndpoint(a->endpoint, status);
      common->unlockShared();
      if (status) a->result.setSuccess(); // Successful query
    } else { // If there was no plugin selected for this endpoint, this will try all possibility
      logger.msg(DEBUG, "The interface of this endpoint (%s) is unspecified, will try all possible plugins", a->endpoint.str());

      // A list for collecting the new endpoints which will be created by copying the original one
      // and setting the InterfaceName for each possible plugins
      std::list<Endpoint> preferredEndpoints;
      std::list<Endpoint> otherEndpoints;
      const std::list<std::string>& preferredInterfaceNames = a->options.getPreferredInterfaceNames();
      
      // A new result object is created for the sub-threads, "true" means we only want to wait for the first successful query
      Result preferredResult(true);
      Result otherResult(true);
      for (std::list<std::string>::const_iterator it = common->getAvailablePlugins().begin(); it != common->getAvailablePlugins().end(); ++it) {
        EntityRetrieverPlugin<T>* plugin = common->load(*it);
        if (!plugin) {
          // Should not happen since all available plugins was already loaded in the constructor.
          // Problem loading the plugin, skip it
          logger.msg(DEBUG, "Problem loading plugin %s, skipping it.", *it);
          continue;
        }
        if (plugin->isEndpointNotSupported(a->endpoint)) {
          logger.msg(DEBUG, "The endpoint (%s) is not supported by this plugin (%s)", a->endpoint.URLString, *it);
          continue;
        }

        // Create a new endpoint with the same endpoint and a specified interface
        Endpoint endpoint = a->endpoint;
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

      /* We wait for the preferred result object. The wait returns in two cases:
       * 1. one sub-thread was succesful
       * 2. all the sub-threads failed
       * Now check which case happens.
       */
      preferredResult.wait();
      if(!common->lockSharedIfValid()) return;
      EndpointQueryingStatus status;
      for (typename std::list<Endpoint>::const_iterator it = preferredEndpoints.begin(); it != preferredEndpoints.end(); it++) {
        status = (*common)->getStatusOfEndpoint(*it);
        if (status) {
          break;
        }
      }
      // Set the status of the original endpoint (the one without the specified interface)
      if (!status) {
        // Wait for the other threads, maybe they were successful
        otherResult.wait();
        typename std::list<Endpoint>::const_iterator it = otherEndpoints.begin();
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

  template class EntityRetriever<Endpoint>;
  template class EntityRetrieverPlugin<Endpoint>;
  template class EntityRetrieverPluginLoader<Endpoint>;
  template<> Logger EntityRetriever<Endpoint>::logger(Logger::getRootLogger(), "ServiceEndpointRetriever");
  template<> const std::string EntityRetrieverPlugin<Endpoint>::kind("HED:ServiceEndpointRetrieverPlugin");
  template<> Logger EntityRetrieverPluginLoader<Endpoint>::logger(Logger::getRootLogger(), "ServiceEndpointRetrieverPluginLoader");

  template class EntityRetriever<ComputingServiceType>;
  template class EntityRetrieverPlugin<ComputingServiceType>;
  template class EntityRetrieverPluginLoader<ComputingServiceType>;
  template<> Logger EntityRetriever<ComputingServiceType>::logger(Logger::getRootLogger(), "TargetInformationRetriever");
  template<> Logger EntityRetrieverPluginLoader<ComputingServiceType>::logger(Logger::getRootLogger(), "TargetInformationRetrieverPluginLoader");
  template<> const std::string TargetInformationRetrieverPlugin::kind("HED:TargetInformationRetrieverPlugin");

  template class EntityRetriever<Job>;
  template class EntityRetrieverPlugin<Job>;
  template class EntityRetrieverPluginLoader<Job>;
  template<> Logger EntityRetriever<Job>::logger(Logger::getRootLogger(), "JobListRetriever");
  template<> const std::string EntityRetrieverPlugin<Job>::kind("HED:JobListRetrieverPlugin");
  template<> Logger EntityRetrieverPluginLoader<Job>::logger(Logger::getRootLogger(), "JobListRetrieverPluginLoader");


} // namespace Arc
