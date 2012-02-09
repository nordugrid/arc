#ifndef __ARC_SERVICEENDPOINTRETRIEVER_H__
#define __ARC_SERVICEENDPOINTRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

class DataStatus;
class Loader;
class MCC_Status;
class Plugin;
class PluginArgument;
class UserConfig;

///
/**
 *
 **/
class RegistryEndpoint {
public:
  std::string Endpoint;
  std::string Type;
};

///
/**
 *
 **/
// Combination of the GLUE 2 Service and Endpoint classes.
class ServiceEndpoint {
public:
  URL EndpointURL;
  std::list<std::string> EndpointCapabilities;
  std::string EndpointInterfaceName;
  std::string HealthState;
  std::string HealthStateInfo;
  std::string QualityLevel;
};

///
/**
 *
 **/
class ServiceEndpointConsumer {
public:
  virtual void addServiceEndpoint(const ServiceEndpoint&) = 0;
};

///
/**
 *
 **/
class ServiceEndpointContainer : public ServiceEndpointConsumer {
public:
  ServiceEndpointContainer() {}
  ~ServiceEndpointContainer() {}

  void addServiceEndpoint(const ServiceEndpoint&) {}
};

///
/**
 *
 **/
class ServiceEndpointStatus {
public:
  bool status;
private:
  //MCC_Status mccstatus;
  //DataStatus datastatus;
};

///
/**
 * Convenience class for retrieving service endpoint information from a registry
 * or index service.
 **/
// This name does not reflect the fact that it queries a registry/index service.
class ServiceEndpointRetriever {
public:
  /**
   * Start querying the registry/index services specified in the 'registries'
   * parameter, and add retrieved service endpoint information to the consumer,
   * 'seConsumer'. Querying the registries will be done in a threaded manner
   * and the constructor is not waiting for these threads to finish.
   **/
  /*
  ServiceEndpointRetriever(const UserConfig& uc,
                           const std::list<RegistryEndpoint>& registries,
                           ServiceEndpointConsumer& seConsumer,
                           bool recursive = false,
                           std::string capabilityFilter = "");
                           */

  //bool isDone() const;
  //ServiceEndpointStatus getStatus(const std::string& registryEndpoint) const {}

private:
  std::map<std::string, ServiceEndpointStatus> status;
};

///
/**
 *
 **/
class ServiceEndpointRetrieverPlugin : public Plugin {
public:
  virtual ServiceEndpointStatus Query(const UserConfig&, const RegistryEndpoint& rEndpoint, ServiceEndpointConsumer&) = 0;
};

/// Class responsible for loading ServiceEndpointRetrieverPlugin plugins
/**
 * The ServiceEndpointRetrieverPlugin objects returned by a
 * ServiceEndpointRetrieverPluginLoader must not be used after the
 * ServiceEndpointRetrieverPluginLoader goes out of scope.
 **/
class ServiceEndpointRetrieverPluginLoader : public Loader {
public:
  ServiceEndpointRetrieverPluginLoader();
  ~ServiceEndpointRetrieverPluginLoader();

  /// Load a new ServiceEndpointRetrieverPlugin
  /**
   * @param name    The name of the ServiceEndpointRetrieverPlugin to load.
   * @return       A pointer to the new ServiceEndpointRetrieverPlugin (NULL on
   *  error).
   **/
  ServiceEndpointRetrieverPlugin* load(const std::string& name);

  /// Retrieve list of loaded ServiceEndpointRetrieverPlugin objects
  /**
   * @return A reference to the list of ServiceEndpointRetrieverPlugin objects.
   **/
  const std::list<ServiceEndpointRetrieverPlugin*>& GetServiceEndpointRetrieverPlugins() const { return plugins; }

private:
  std::list<ServiceEndpointRetrieverPlugin*> plugins;
};

class ServiceEndpointRetrieverTESTControl {
public:
  static int* tcTimeout;
  static ServiceEndpointStatus* tcStatus;
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVER_H__
