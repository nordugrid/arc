#ifndef __ARC_COMPUTINGSERVICERETRIEVER_H__
#define __ARC_COMPUTINGSERVICERETRIEVER_H__

#include <list>
#include <string>

#include <arc/UserConfig.h>
#include <arc/client/Endpoint.h>
#include <arc/client/EntityRetriever.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {

class ComputingServiceRetriever : public EntityConsumer<Endpoint>, public EntityContainer<ComputingServiceType> {
public:
  ComputingServiceRetriever(
    const UserConfig& uc,
    const std::list<Endpoint>& services = std::list<Endpoint>(),
    const std::list<std::string>& rejectedServices = std::list<std::string>(),
    const std::list<std::string>& preferredInterfaceNames = std::list<std::string>(),
    const std::list<std::string>& capabilityFilter = std::list<std::string>(1, Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO))
  );

  void wait() { ser.wait(); tir.wait(); }

  void addEndpoint(const Endpoint& service);
  void addEntity(const Endpoint& service) { addEndpoint(service); }
  
  void addConsumer(EntityConsumer<ComputingServiceType>& c) { tir.addConsumer(c); };
  void removeConsumer(const EntityConsumer<ComputingServiceType>& c) { tir.removeConsumer(c); }
  
  void GetExecutionTargets(std::list<ExecutionTarget>& etList) {
    ExecutionTarget::GetExecutionTargets(*this, etList);
  }

private:
  ServiceEndpointRetriever ser;
  TargetInformationRetriever tir;
};

} // namespace Arc

#endif // __ARC_COMPUTINGSERVICERETRIEVER_H__
