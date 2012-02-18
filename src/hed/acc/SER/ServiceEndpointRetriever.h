#ifndef __ARC_SERVICEENDPOINTRETRIEVER_H__
#define __ARC_SERVICEENDPOINTRETRIEVER_H__

#include <algorithm>
#include <list>
#include <map>
#include <string>

#include <arc/URL.h>
#include <arc/Thread.h>
#include <arc/UserConfig.h>

#include "ServiceEndpointRetrieverPlugin.h"

namespace Arc {

class DataStatus;
class MCC_Status;

///
/**
 *
 **/
// Combination of the GLUE 2 Service and Endpoint classes.
class ServiceEndpoint {
public:
  ServiceEndpoint(std::string EndpointURL = "",
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

  std::string EndpointURL;
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
  RegistryEndpoint(std::string Endpoint = "", std::string InterfaceName = "") : Endpoint(Endpoint), InterfaceName(InterfaceName) {}
  RegistryEndpoint(ServiceEndpoint service) : Endpoint(service.EndpointURL), InterfaceName(service.EndpointInterfaceName) {}

  static bool isRegistry(ServiceEndpoint service) {
    return (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), "information.discovery.registry") != service.EndpointCapabilities.end());
  }

  std::string str() const {
    return Endpoint + " (" + (InterfaceName.empty() ? "<unspecified>" : InterfaceName) + ")";
  }

  // Needed for std::map ('statuses' in ServiceEndpointRetriever) to be able to sort the keys
  bool operator<(const RegistryEndpoint& other) const {
    return Endpoint + InterfaceName < other.Endpoint + other.InterfaceName;
  }

  std::string Endpoint;
  std::string InterfaceName;
};


///
/**
 *
 **/
class ServiceEndpointConsumer {
public:
  virtual void addServiceEndpoint(const ServiceEndpoint&) = 0;
};

class RegistryEndpointConsumer {
public:
  virtual void addRegistryEndpoint(const RegistryEndpoint&) = 0;
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

enum SERStatus { SER_UNKNOWN, SER_STARTED, SER_FAILED, SER_NOPLUGIN, SER_SUCCESSFUL };

//! Conversion to string.
/*! Conversion from SERStatus to string.
  @param s The SERStatus to convert.
*/
std::string string(SERStatus s);

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
class ServiceEndpointRetriever : RegistryEndpointConsumer, ServiceEndpointConsumer {
public:
  /**
   * Start querying the registry/index services specified in the 'registries'
   * parameter, and add retrieved service endpoint information to the consumer,
   * 'seConsumer'. Querying the registries will be done in a threaded manner
   * and the constructor is not waiting for these threads to finish.
   **/
  ServiceEndpointRetriever(const UserConfig& uc,
                           ServiceEndpointConsumer& consumer,
                           bool recursive = false,
                           std::list<std::string> capabilityFilter = std::list<std::string>());
  ~ServiceEndpointRetriever();
  void wait() const;
  bool isDone() const;
  void stopSendingEndpoints();

  RegistryEndpointStatus getStatusOfRegistry(RegistryEndpoint) const;
  bool setStatusOfRegistry(const RegistryEndpoint&, const RegistryEndpointStatus&, bool overwrite = true);

  virtual void addServiceEndpoint(const ServiceEndpoint&);
  virtual void addRegistryEndpoint(const RegistryEndpoint&);

private:
  static void queryRegistry(void *arg_);

  class SERCommon {
  public:
    SERCommon() : mutex(), isActive(true), loader() {};
    SharedMutex mutex;
    bool isActive;
    ServiceEndpointRetrieverPluginLoader loader;
  };
  ThreadedPointer<SERCommon> serCommon;

  class ThreadArgSER {
  public:
    ThreadArgSER(const UserConfig& uc,
                 const ThreadedPointer<SERCommon>& serCommon,
                 const ThreadedPointer<SimpleCounter>& threadCounter)
      : uc(uc), serCommon(serCommon), subthread(false), threadCounter(threadCounter) {};

    const UserConfig& uc;
    RegistryEndpoint registry;
    std::string pluginName;
    ServiceEndpointRetriever* ser;
    ThreadedPointer<SERCommon> serCommon;
    bool subthread;
    ThreadedPointer<SimpleCounter> threadCounter;
  };

  std::map<RegistryEndpoint, RegistryEndpointStatus> statuses;

  static Logger logger;
  const UserConfig& uc;
  ServiceEndpointConsumer& consumer;
  bool recursive;
  std::list<std::string> capabilityFilter;

  mutable SimpleCondition lock;
  ThreadedPointer<SimpleCounter> threadCounter;
  std::map<std::string, std::string> interfacePluginMap;
};

class ServiceEndpointRetrieverTESTControl {
public:
  static float delay;
  static RegistryEndpointStatus status;
  static std::list<ServiceEndpoint> endpoints;
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVER_H__
