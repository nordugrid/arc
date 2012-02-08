namespace Arc {

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
  void addServiceEndpoint(const ServiceEndpoint&) = 0;
};

///
/**
 *
 **/
class ServiceEndpointContainer : public ServiceEndpointConsumer {
public:
  void addServiceEndpoint(const ServiceEndpoint&) {}
};

}

///
/**
 *
 **/
class ServiceEndpointStatus {
public:

private:
  MCC_Status mccstatus;
  DataStatus datastatus;
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
  ServiceEndpointRetriever(const std::list<RegistryEndpoint>& registries,
                           ServiceEndpointConsumer& seConsumer,
                           bool recursive = false,
                           std::string capabilityFilter = "");

  bool isDone() const;
  ServiceEndpointStatus getStatus(const std::string& registryEndpoint) const;

private:
  std::map<std::string, ServiceEndpointStatus> status;
};

///
/**
 *
 **/
class ServiceEndpointRetrieverPlugin : public Plugin {
public:

};
