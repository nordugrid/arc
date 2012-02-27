#ifndef __ARC_JOBLISTRETRIEVER_H__
#define __ARC_JOBLISTRETRIEVER_H__

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

template class EndpointRetriever<ComputingInfoEndpoint, Job>;
template class EndpointRetrieverPlugin<ComputingInfoEndpoint, Job>;
template class EndpointRetrieverPluginLoader<ComputingInfoEndpoint, Job>;

typedef EndpointRetriever<ComputingInfoEndpoint, Job>             JobListRetriever;
typedef EndpointRetrieverPlugin<ComputingInfoEndpoint, Job>       JobListRetrieverPlugin;
typedef EndpointRetrieverPluginLoader<ComputingInfoEndpoint, Job> JobListRetrieverPluginLoader;

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVER_H__
