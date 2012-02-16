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

    if (itPluginName == interfacePluginMap.end()) {
      return false;
    }

    /* TODO:
    else {
      logger.msg(DEBUG, "Registry endpoint has no type, will try all possible plugins: " + it->str());
      for (std::list<std::string>::const_iterator it2 = types.begin(); it2 != types.end(); ++it2) {
        RegistryEndpoint registry = *it;
        //registry.InterfaceName = *it2; // TODO: Should be fixed in another way.
        logger.msg(Arc::DEBUG, "New registry endpoint is created from the typeless one: " + registry.str());
        //createThread(registry, ""); // TODO: Should be fixed in another way.
      }
    }
    */

    ThreadArgSER *arg = new ThreadArgSER(uc, serCommon);
    arg->registry = registry;
    arg->pluginName = itPluginName->second;
    arg->capabilityFilter = capabilityFilter;
    arg->ser = this;
    logger.msg(Arc::DEBUG, "Starting thread to query the registry on " + arg->registry.str());
    if (!CreateThreadFunction(&queryRegistry, arg, &threadCounter)) {
      logger.msg(Arc::ERROR, "Failed to start querying the registry on " + arg->registry.str() + " (unable to create thread)");
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
      capabilityFilter(capabilityFilter)
  {
    // Used for holding names of all available plugins.
    std::list<std::string> types(serCommon->loader.getListOfPlugins());
    // Map supported interfaces to available plugins.
    for (std::list<std::string>::const_iterator itT = types.begin(); itT != types.end(); ++itT) {
      ServiceEndpointRetrieverPlugin* p = serCommon->loader.load(*itT);
      for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
        // If two plugins supports two identical interfaces, then only the last will appear in the map.
        interfacePluginMap[*itI] = *itT;
      }
    }

    for (std::list<RegistryEndpoint>::const_iterator it = registries.begin(); it != registries.end(); ++it) {
      createThread(*it);
    }
  }

  ServiceEndpointRetriever::~ServiceEndpointRetriever() {
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
    threadCounter.wait();
  };

  bool ServiceEndpointRetriever::isDone() const {
    return threadCounter.get() == 0;
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
      logger.msg(DEBUG, "Setting status (%s) for registry (%s)", string(status.status), registry.Endpoint);
      statuses[registry] = status;
      wasSet = true;
    }
    lock.unlock();
    return wasSet;
  };

  void ServiceEndpointRetriever::queryRegistry(void *arg) {
    ThreadArgSER* a = (ThreadArgSER*)arg;

    ThreadedPointer<SERCommon>& serCommon = a->serCommon;
    ServiceEndpointRetrieverPlugin* plugin = serCommon->loader.load(a->pluginName);

    if (plugin) {
      RegistryEndpointStatus status(SER_STARTED);

      serCommon->mutex.lockShared();
      if (!serCommon->isActive) {
        serCommon->mutex.unlockShared();
        delete a;
        return;
      }
      bool wasSet = a->ser->setStatusOfRegistry(a->registry, status, false);
      serCommon->mutex.unlockShared();

      if (wasSet) {
        logger.msg(Arc::DEBUG, "Calling plugin to query registry on " + a->registry.str());

        std::list<ServiceEndpoint> endpoints;
        status = plugin->Query(a->uc, a->registry, endpoints);

        serCommon->mutex.lockShared();
        if (!serCommon->isActive) {
          serCommon->mutex.unlockShared();
          delete a;
          return;
        }
        for (std::list<ServiceEndpoint>::iterator it = endpoints.begin(); it != endpoints.end(); it++) {
          a->ser->addServiceEndpoint(*it);
        }
        a->ser->setStatusOfRegistry(a->registry, status);
        serCommon->mutex.unlockShared();

      } else {
        logger.msg(Arc::DEBUG, "Will not query registry, because another thread is already querying it: " + a->registry.str());
      }
    }
    delete a;
  }




  // TESTControl

  float ServiceEndpointRetrieverTESTControl::delay = 0;
  RegistryEndpointStatus ServiceEndpointRetrieverTESTControl::status;
  std::list<ServiceEndpoint> ServiceEndpointRetrieverTESTControl::endpoints = std::list<ServiceEndpoint>();
}
