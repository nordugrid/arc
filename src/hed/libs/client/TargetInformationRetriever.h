#ifndef __ARC_TARGETINFORMATIONRETRIEVER_H__
#define __ARC_TARGETINFORMATIONRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/client/EndpointRetriever.h>
#include <arc/client/ServiceEndpointRetriever.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/loader/Loader.h>

namespace Arc {

class EndpointQueryingStatus;
class ExecutionTarget;

class ComputingInfoEndpoint {
public:
  ComputingInfoEndpoint(std::string Endpoint = "", std::string InterfaceName = "") : Endpoint(Endpoint), InterfaceName(InterfaceName) {}
  ComputingInfoEndpoint(ServiceEndpoint service) : Endpoint(service.EndpointURL), InterfaceName(service.EndpointInterfaceName) {}

  static bool isComputingInfo(ServiceEndpoint service) {
    return (std::find(service.EndpointCapabilities.begin(), service.EndpointCapabilities.end(), ComputingInfoCapability) != service.EndpointCapabilities.end());
  }

  std::string str() const {
    return Endpoint + " (" + (InterfaceName.empty() ? "<unspecified>" : InterfaceName) + ")";
  }

  // Needed for std::map ('statuses' in TargetInformationRetriever) to be able to sort the keys
  bool operator<(const ComputingInfoEndpoint& other) const {
    return Endpoint + InterfaceName < other.Endpoint + other.InterfaceName;
  }

  std::string Endpoint;
  std::string InterfaceName;
  static const std::string ComputingInfoCapability;
};

template class EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget>;
template class EndpointRetrieverPlugin<ComputingInfoEndpoint, ExecutionTarget>;
template class EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget>;

typedef EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget> TargetInformationRetriever;
typedef EndpointRetrieverPlugin<ComputingInfoEndpoint, ExecutionTarget> TargetInformationRetrieverPlugin;
typedef EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget> TargetInformationRetrieverPluginLoader;

class TargetInformationRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<ExecutionTarget> targets;
  static EndpointQueryingStatus status;
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVER_H__
