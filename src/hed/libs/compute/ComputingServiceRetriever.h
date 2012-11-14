#ifndef __ARC_COMPUTINGSERVICERETRIEVER_H__
#define __ARC_COMPUTINGSERVICERETRIEVER_H__

#include <list>
#include <string>

#include <arc/UserConfig.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/EntityRetriever.h>
#include <arc/compute/ExecutionTarget.h>

namespace Arc {
  
class ComputingServiceUniq : public EntityConsumer<ComputingServiceType> {
public:
  void addEntity(const ComputingServiceType& service);
  std::list<ComputingServiceType> getServices() { return services; }
private:
  std::list<ComputingServiceType> services;
  
  static Logger logger;
};

/// Retrieves information about computing elements by querying service registries and CE information systems
/** The ComputingServiceRetriever queries service registries and local information systems
    of computing elements, creates ComputingServiceType objects from the retrieved information
    and besides storing those objects also sends them to all the registered consumers.
*/
class ComputingServiceRetriever : public EntityConsumer<Endpoint>, public EntityContainer<ComputingServiceType> {
public:
  
  /// Creates a ComputingServiceRetriever with a list of services to query
  /**
  
      \param[in] uc the UserConfig object containing the credentails to use for connecting services
      \param[in] services a list of Endpoint objects containing the services (registries or CEs) to query
      \param[in] rejectedServices if the URL of a service matches an element in this list,
      the service will not be queried
      \param[in] preferredInterfaceNames when an Endpoint does not have it's GLUE2 interface name specified
      the class will try interfaces specified here first, and if they return no results,
      then all the other possible interfaces are tried
      \param[in] capabilityFilter only those ComputingServiceType objects will be sent to the consumer which has
      at least one of the capabilities provided here
  */
  ComputingServiceRetriever(
    const UserConfig& uc,
    const std::list<Endpoint>& services = std::list<Endpoint>(),
    const std::list<std::string>& rejectedServices = std::list<std::string>(),
    const std::set<std::string>& preferredInterfaceNames = std::set<std::string>(),
    const std::list<std::string>& capabilityFilter = std::list<std::string>(1, Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO))
  );

  /// Waits for all the results to arrive
  /**
      This method call will only return when all the results have arrived..
  */
  void wait() { ser.wait(); tir.wait(); }

  /// Adds a new service (registry or computing element) to query
  /**
      Depending on the type of the service, it will be added to the internal ServiceEndpointRetriever
      (if it's a registry) or the internal TargetInformationRetriever (if it's a computing element).
      \param[in] service an Endpoint refering to a service to query
  */
  void addEndpoint(const Endpoint& service);
      
  /// Adds a new service to query (used by the internal ServiceEndpointRetriever)
  /**
      The internal ServiceEndpointRetriever queries the service registries and feeds the results
      back to the ComputingServiceRetriever through this method, so the ComputingServiceRetriever
      can recursively query the found resources.
      \param[in] service an Endpoint refering to a service to query
  */
  void addEntity(const Endpoint& service) { addEndpoint(service); }
  
  /// Add a consumer to the ComputingServiceRetriever which will get the results
  /**
      All the consumers will receive all the retrieved ComputingServiceType objects one by one.
      \param[in] c one consumer of the type EntityConsumer<ComputingServiceType>
      capable of accepting ComputingServiceType objects
  */
  void addConsumer(EntityConsumer<ComputingServiceType>& c) { tir.addConsumer(c); };
  
  /// Remove a previously added consumer from this ComputingServiceRetriever
  /**
      The removed consumer will not get any more result objects
      \param[in] c the consumer to be removed
  */
  void removeConsumer(const EntityConsumer<ComputingServiceType>& c) { tir.removeConsumer(c); }
  
  /// Convenience method to generate ExectionTarget objects from the resulted ComputingServiceType objects
  /**
      Calls the class method ExectuonTarget::GetExecutionTargets with the
      list of retrieved ComputerServiceType objects.
      \param[out] etList the generated ExecutionTargets will be put into this list
  */
  void GetExecutionTargets(std::list<ExecutionTarget>& etList) {
    ExecutionTarget::GetExecutionTargets(*this, etList);
  }

  /// Get status of all the queried Endpoint objects
  /**
   * This method returns a copy of the internal status map, and thus is only
   * a snapshot. If you want the final status map, make sure to invoke the
   * ComputingServiceRetriever::wait method before this one.
   * \return a map with Endpoint objects as keys and status objects as values.
   **/
  std::map<Endpoint, EndpointQueryingStatus> getAllStatuses() const { std::map<Endpoint, EndpointQueryingStatus> s = ser.getAllStatuses(), t = tir.getAllStatuses(); s.insert(t.begin(), t.end()); return s; }

private:
  ServiceEndpointRetriever ser;
  TargetInformationRetriever tir;

  static Logger logger;
};

} // namespace Arc

#endif // __ARC_COMPUTINGSERVICERETRIEVER_H__
