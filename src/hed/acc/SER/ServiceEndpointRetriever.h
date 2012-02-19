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
    return (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), RegistryCapability) != service.EndpointCapabilities.end());
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
  static const std::string RegistryCapability;
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
                           bool recursive = false,
                           std::list<std::string> capabilityFilter = std::list<std::string>());
  ~ServiceEndpointRetriever() { serCommon->deactivate(); }

  void wait() const { serResult.wait(); };
  bool isDone() const { return serResult.wait(0); };

  void addConsumer(ServiceEndpointConsumer& c) { consumerLock.lock(); consumers.push_back(&c); consumerLock.unlock(); }
  void removeConsumer(const ServiceEndpointConsumer&);

  RegistryEndpointStatus getStatusOfRegistry(RegistryEndpoint) const;
  bool setStatusOfRegistry(const RegistryEndpoint&, const RegistryEndpointStatus&, bool overwrite = true);

  virtual void addServiceEndpoint(const ServiceEndpoint&);
  virtual void addRegistryEndpoint(const RegistryEndpoint&);

private:
  static void queryRegistry(void *arg_);

  // Common configuration part
  class SERCommon {
  public:
    SERCommon(ServiceEndpointRetriever* s, const UserConfig& u) :
      mutex(), ser(s), loader(), uc(u) {}; // Maybe full copy of UserConfig wouldbe safer?
    void deactivate(void) {
      mutex.lockExclusive();
      ser = NULL;
      mutex.unlockExclusive();
    }
    ServiceEndpointRetriever* lockExclusive(void) {
      mutex.lockExclusive();
      if(ser) return ser;
      mutex.unlockExclusive();
    }
    void unlockExclusive(void) { mutex.unlockExclusive(); }
    ServiceEndpointRetriever* lockShared(void) {
      mutex.lockShared();
      if(ser) return ser;
      mutex.unlockShared();
    }
    void unlockShared(void) { mutex.unlockShared(); }
    const UserConfig& Cfg(void) const { return uc; }
    ServiceEndpointRetrieverPluginLoader& Loader(void) { return loader; }
  private:
    SharedMutex mutex;
    ServiceEndpointRetriever* ser;
    ServiceEndpointRetrieverPluginLoader loader;
    const UserConfig& uc;
  };

  // Represents completeness of queriies run in threads.
  // Different implementations are meant for waiting for either one or all threads.
  // TODO: counter is duplicate in this implimentation. It may be simplified
  //       either by using counter of ThreadedPointer or implementing part of
  //       ThreadedPointer directly.
  class SERResult: private ThreadedPointer<SimpleCounter>  {
  public:
    // Creates initial instance
    SERResult(bool one_success = false):
      ThreadedPointer<SimpleCounter>(new SimpleCounter),
      need_one_success(false),success(false) { };
    // Creates new reference representing query - increments counter
    SERResult(const SERResult& r):
      ThreadedPointer<SimpleCounter>(r),
      need_one_success(r.need_one_success),success(false) {
      Ptr()->inc();
    };
    // Query finished - decrement or reset counter (if one result is enough)
    ~SERResult(void) {
      if(need_one_success && success) {
        Ptr()->set(0);
      } else {
        Ptr()->dec();
      };
    };
    // Mark this result as successful (failure by default)
    void setSuccess(void) { success = true; };
    // Wait for queries to finish
    bool wait(int t = -1) const { return Ptr()->wait(t); };
    int get(void) { return Ptr()->get(); };
  private:
    bool success;
    bool need_one_success;
  };

  ThreadedPointer<SERCommon> serCommon;
  SERResult serResult;

  class ThreadArgSER {
  public:
    ThreadArgSER(const ThreadedPointer<SERCommon>& serCommon,
                 SERResult& serResult)
      : serCommon(serCommon), serResult(serResult) {};
    ThreadArgSER(const ThreadArgSER& v,
                 SERResult& serResult)
      : serCommon(v.serCommon), serResult(serResult),
        registry(v.registry), pluginName(v.pluginName),
        capabilityFilter(v.capabilityFilter) {};
    // Objects for communication with caller
    ThreadedPointer<SERCommon> serCommon;
    SERResult serResult;
    // Per-thread parameters
    RegistryEndpoint registry;
    std::string pluginName;
    std::list<std::string> capabilityFilter;
  };

  std::map<RegistryEndpoint, RegistryEndpointStatus> statuses;

  static Logger logger;
  const UserConfig& uc;
  std::list<ServiceEndpointConsumer*> consumers;
  bool recursive;
  std::list<std::string> capabilityFilter;

  mutable SimpleCondition consumerLock;
  mutable SimpleCondition statusLock;
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
