#ifndef __ARC_SERVICEENDPOINTRETRIEVER_H__
#define __ARC_SERVICEENDPOINTRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>
#include <arc/UserConfig.h>

namespace Arc {

class DataStatus;
class Loader;
class MCC_Status;
class Plugin;
class PluginArgument;

///
/**
 *
 **/
// Combination of the GLUE 2 Service and Endpoint classes.
class ServiceEndpoint {
public:
  ServiceEndpoint(URL EndpointURL = URL(),
                  std::list<std::string> EndpointCapabilities = std::list<std::string>(),
                  std::string EndpointInterfaceName = "",
                  std::string HealthState = "",
                  std::string HealthStateInfo = "",
                  std::string QualityLevel = "")
    : EndpointURL(EndpointURL),
    EndpointCapabilities(EndpointCapabilities),
    EndpointInterfaceName(EndpointInterfaceName),
    HealthState(HealthState),
    HealthStateInfo(HealthStateInfo),
    QualityLevel(QualityLevel) {}
                      
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
class RegistryEndpoint {
public:
  RegistryEndpoint(std::string Endpoint = "", std::string Type = "") : Endpoint(Endpoint), Type(Type) {}
  
  RegistryEndpoint(ServiceEndpoint service) {
    Endpoint = service.EndpointURL.str();
    if (service.EndpointInterfaceName == "org.nordugrid.ldapegiis") {
      Type = "EGIIS";
    } else if (service.EndpointInterfaceName == "org.ogf.emir") {
      Type = "EMIR";
    } else if (service.EndpointInterfaceName == "org.nordugrid.test") {
      Type = "TEST";
    }
  }
  
  static bool isRegistry(ServiceEndpoint service) {
    return (std::count(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), "information.discovery.registry") != 0);
  }
  
  std::string str() const {
    return Endpoint + " (" + Type + ")";
  }
  
  // Needed for std::map ('statuses' in ServiceEndpointRetriever) to be able to sort the keys
  bool operator<(const RegistryEndpoint& other) const {
    return Endpoint + Type < other.Endpoint + other.Type;
  }

  std::string Endpoint;
  std::string Type;
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

class ThreadArgSER;

///
/**
 * Convenience class for retrieving service endpoint information from a registry
 * or index service.
 **/
// This name does not reflect the fact that it queries a registry/index service.
class ServiceEndpointRetriever : ServiceEndpointConsumer {
public:
  /**
   * Start querying the registry/index services specified in the 'registries'
   * parameter, and add retrieved service endpoint information to the consumer,
   * 'seConsumer'. Querying the registries will be done in a threaded manner
   * and the constructor is not waiting for these threads to finish.
   **/
  ServiceEndpointRetriever(UserConfig userconfig,
                           std::list<RegistryEndpoint> registries,
                           ServiceEndpointConsumer& consumer,
                           bool recursive = false,
                           std::list<std::string> capabilityFilter = std::list<std::string>());
  void wait() const;  
  bool isDone() const;
  
  RegistryEndpointStatus getStatusOfRegistry(RegistryEndpoint) const;
  bool testAndSetStatusOfRegistry(RegistryEndpoint, RegistryEndpointStatus);
  void setStatusOfRegistry(RegistryEndpoint, RegistryEndpointStatus);
  
  virtual void addServiceEndpoint(const ServiceEndpoint&);
    
private:
  static void queryRegistry(void *arg_);
  
  bool createThread(RegistryEndpoint registry);

  std::map<RegistryEndpoint, RegistryEndpointStatus> statuses;

  static Logger logger;
  mutable SimpleCounter threadCounter;
  mutable SimpleCondition lock;
  UserConfig userconfig;
  ServiceEndpointConsumer& consumer;
  bool recursive;
  std::list<std::string> capabilityFilter;
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
  
  std::list<std::string> getListOfPlugins();

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
  static float delay;
  static RegistryEndpointStatus status;
  static std::list<ServiceEndpoint> endpoints;
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVER_H__
