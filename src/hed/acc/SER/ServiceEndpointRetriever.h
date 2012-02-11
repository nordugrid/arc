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
  RegistryEndpoint(std::string Endpoint = "", std::string Type = "") : Endpoint(Endpoint), Type(Type) {};
  std::string Endpoint;
  std::string Type;
  // Needed for std::map ('status' in ServiceEndpointRetriever) to be able to sort the keys
  bool operator<(const RegistryEndpoint& other) const {
    return Endpoint + Type < other.Endpoint + other.Type;
  }
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

  void addServiceEndpoint(const ServiceEndpoint& endpoint) { endpoints.push_back(endpoint); }
  std::list<ServiceEndpoint> endpoints;
};

///
/**
 *
 **/
   
enum SERStatus { SER_UNKNOWN, SER_STARTED, SER_FAILED, SER_SUCCESSFUL };

class RegistryEndpointStatus {
public:
  RegistryEndpointStatus(SERStatus status = SER_UNKNOWN) : status(status) {};
  SERStatus status;
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
  ServiceEndpointRetriever(const UserConfig& uc,
                           const std::list<RegistryEndpoint>& registries,
                           ServiceEndpointConsumer& seConsumer,
                           const bool recursive = false,
                           const std::list<std::string> capabilityFilter = std::list<std::string>());
  
  void wait() const {}  
  bool isDone() const { return true; }
  RegistryEndpointStatus getStatusOfRegistry(const RegistryEndpoint) const;

private:
  std::map<RegistryEndpoint, RegistryEndpointStatus> status;
  
  static Logger logger;
};

///
/**
 *
 **/
class ServiceEndpointRetrieverPlugin : public Plugin {
public:
  virtual RegistryEndpointStatus Query(const UserConfig&, const RegistryEndpoint& rEndpoint, ServiceEndpointConsumer&) = 0;
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
  static float tcPeriod;
  static RegistryEndpointStatus tcStatus;
  static std::list<ServiceEndpoint> tcEndpoints;
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVER_H__
