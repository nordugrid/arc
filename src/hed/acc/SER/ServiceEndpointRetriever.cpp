// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/loader/FinderLoader.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

  Logger ServiceEndpointRetriever::logger(Logger::getRootLogger(), "ServiceEndpointRetriever");

  class ThreadArgSER {
  public:
    UserConfig userconfig;
    RegistryEndpoint registry;
    std::list<std::string> capabilityFilter;
    ServiceEndpointRetriever* ser;
  };
  
  bool ServiceEndpointRetriever::createThread(RegistryEndpoint& registry) {
    ThreadArgSER *arg = new ThreadArgSER;
    arg->userconfig = userconfig;
    arg->registry = registry;
    arg->capabilityFilter = capabilityFilter;
    arg->ser = this;
    std::string registryString = arg->registry.Endpoint + " (" + arg->registry.Type + ")";
    logger.msg(Arc::DEBUG, "Starting thread to query the registry on " + registryString);
    if (!CreateThreadFunction(&queryRegistry, arg, &threadCounter)) {
      logger.msg(Arc::ERROR, "Failed to start querying the registry on " + registryString + " (unable to create thread)");
      delete arg;
      return false;
    }
    return true;
  }

  ServiceEndpointRetriever::ServiceEndpointRetriever(UserConfig userconfig,
                                                     std::list<RegistryEndpoint> registries,
                                                     ServiceEndpointConsumer& consumer,
                                                     bool recursive,
                                                     std::list<std::string> capabilityFilter)
  : userconfig(userconfig), consumer(consumer), recursive(recursive), capabilityFilter(capabilityFilter) {
    for (std::list<RegistryEndpoint>::iterator it = registries.begin(); it != registries.end(); it++) {
      createThread(*it);
    }
  }
  
  void ServiceEndpointRetriever::addServiceEndpoint(const ServiceEndpoint& endpoint) {
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
    ServiceEndpointRetrieverPluginLoader loader;
    ServiceEndpointRetrieverPlugin* plugin = loader.load(a->registry.Type);
    if (plugin) {
      RegistryEndpointStatus status(SER_STARTED);
      bool wasSet = a->ser->testAndSetStatusOfRegistry(a->registry, status);
      if (wasSet) {
        logger.msg(Arc::DEBUG, "Querying " + a->registry.Endpoint);
        status = plugin->Query(a->userconfig, a->registry, *a->ser);
        a->ser->setStatusOfRegistry(a->registry, status);
      } else {
        logger.msg(Arc::DEBUG, "Will not query registry, because another thread is already querying it: " + a->registry.Endpoint);
      }
    }
    delete a;
  }
  


  
  // TESTControl

  float ServiceEndpointRetrieverTESTControl::delay = 0;
  RegistryEndpointStatus ServiceEndpointRetrieverTESTControl::status;
  std::list<ServiceEndpoint> ServiceEndpointRetrieverTESTControl::endpoints = std::list<ServiceEndpoint>();

  // PluginLoader

  ServiceEndpointRetrieverPluginLoader::ServiceEndpointRetrieverPluginLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  ServiceEndpointRetrieverPluginLoader::~ServiceEndpointRetrieverPluginLoader() {
    for (std::list<ServiceEndpointRetrieverPlugin*>::iterator it = plugins.begin();
         it != plugins.end(); it++) {
      delete *it;
    }
  }

  ServiceEndpointRetrieverPlugin* ServiceEndpointRetrieverPluginLoader::load(const std::string& name) {
    if (name.empty()) {
      return NULL;
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(), "HED:ServiceEndpointRetrieverPlugin", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for %s plugin is installed", name, name);
      logger.msg(DEBUG, "ServiceEndpointRetrieverPlugin plugin \"%s\" not found.", name, name);
      return NULL;
    }

    ServiceEndpointRetrieverPlugin *p = factory_->GetInstance<ServiceEndpointRetrieverPlugin>("HED:ServiceEndpointRetrieverPlugin", name, NULL, false);

    if (!p) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "ServiceEndpointRetrieverPlugin %s could not be created.", name, name);
      return NULL;
    }

    plugins.push_back(p);
    logger.msg(DEBUG, "Loaded ServiceEndpointRetrieverPlugin %s", name);
    return p;
  }

}
