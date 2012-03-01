#ifndef __ARC_JOBLISTRETRIEVERPLUGINTEST_H__
#define __ARC_JOBLISTRETRIEVERPLUGINTEST_H__

#include <arc/client/EndpointRetriever.h>

namespace Arc {

class JobListRetrieverPluginTEST : public EndpointRetrieverPlugin<ComputingInfoEndpoint, Job> {
public:
  JobListRetrieverPluginTEST() { supportedInterfaces.push_back("org.nordugrid.jlrtest"); }
  ~JobListRetrieverPluginTEST() {}

  static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginTEST; }
  virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
  virtual bool isEndpointNotSupported(const ComputingInfoEndpoint& endpoint) const { return endpoint.URLString.empty(); }
};

}

#endif // __ARC_JOBLISTRETRIEVERPLUGINTEST_H__
