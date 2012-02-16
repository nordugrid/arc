// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

  Logger ServiceEndpointRetriever::logger(Logger::getRootLogger(), "ServiceEndpointRetriever");

  bool ServiceEndpointRetriever::createThread(RegistryEndpoint registry) {
    ThreadArgSER *arg = new ThreadArgSER(uc, serCommon);
    arg->registry = registry;
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
    std::list<std::string> types;
    for (std::list<RegistryEndpoint>::iterator it = registries.begin(); it != registries.end(); ++it) {
      if (it->Type.empty()) {
        logger.msg(DEBUG, "Registry endpoint has no type, will try all possible plugins: " + it->str());
        if (types.empty()) {
          types = serCommon->serpl.getListOfPlugins();
        }
        for (std::list<std::string>::const_iterator it2 = types.begin(); it2 != types.end(); ++it2) {
          RegistryEndpoint registry = *it;
          registry.Type = *it2;
          logger.msg(Arc::DEBUG, "New registry endpoint is created from the typeless one: " + registry.str());
          createThread(registry);
        }
      } else {
        createThread(*it);
      }
    }
  }

  ServiceEndpointRetriever::~ServiceEndpointRetriever() {
    serCommon->mutex.lockExclusive();
    serCommon->isActive = false;
    serCommon->mutex.unlockExclusive();
  }

  void ServiceEndpointRetriever::addServiceEndpoint(const ServiceEndpoint& endpoint) {
    if (recursive) {
      if (RegistryEndpoint::isRegistry(endpoint)) {
        RegistryEndpoint registry(endpoint);
        logger.msg(Arc::DEBUG, "Found a registry, will query it recursively: " + registry.str());
        createThread(registry);
      }
    }
    bool match = false;
    if (capabilityFilter.empty()) {
      match = true;
    } else {
      for (std::list<std::string>::iterator it = capabilityFilter.begin(); it != capabilityFilter.end(); it++) {
        if (std::count(endpoint.EndpointCapabilities.begin(), endpoint.EndpointCapabilities.end(), *it)) {
          match = true;
        }
      }
    }
    if (match) {
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

  bool ServiceEndpointRetriever::testAndSetStatusOfRegistry(RegistryEndpoint registry, RegistryEndpointStatus status) {
    lock.lock();
    std::map<RegistryEndpoint, RegistryEndpointStatus>::const_iterator it = statuses.find(registry);
    bool wasSet = false;
    // If this registry is not yet in the map
    if (it == statuses.end()) {
      statuses[registry] = status;
      wasSet = true;
    }
    lock.unlock();
    return wasSet;
  };

  void ServiceEndpointRetriever::setStatusOfRegistry(RegistryEndpoint registry, RegistryEndpointStatus status) {
    lock.lock();
    statuses[registry] = status;
    lock.unlock();
  };

  void ServiceEndpointRetriever::queryRegistry(void *arg) {
    ThreadArgSER* a = (ThreadArgSER*)arg;

    ThreadedPointer<SERCommon>& serCommon = a->serCommon;
    ServiceEndpointRetrieverPlugin* plugin = serCommon->serpl.load(a->registry.Type);

    if (plugin) {
      RegistryEndpointStatus status(SER_STARTED);

      serCommon->mutex.lockShared();
      if (!serCommon->isActive) {
        serCommon->mutex.unlockShared();
        delete a;
        return;
      }
      bool wasSet = a->ser->testAndSetStatusOfRegistry(a->registry, status);
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
