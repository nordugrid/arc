#ifndef __ARC_TARGETINFORMATIONRETRIEVER_H__
#define __ARC_TARGETINFORMATIONRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/client/Endpoint.h>
#include <arc/client/EndpointRetriever.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/loader/Loader.h>

namespace Arc {

class EndpointQueryingStatus;
class ExecutionTarget;

template class EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget>;
template class EndpointRetrieverPlugin<ComputingInfoEndpoint, ExecutionTarget>;
template class EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget>;

typedef EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget> TargetInformationRetriever;
typedef EndpointRetrieverPlugin<ComputingInfoEndpoint, ExecutionTarget> TargetInformationRetrieverPlugin;
typedef EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget> TargetInformationRetrieverPluginLoader;

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVER_H__
