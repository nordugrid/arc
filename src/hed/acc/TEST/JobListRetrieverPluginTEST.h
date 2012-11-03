#ifndef __ARC_JOBLISTRETRIEVERPLUGINTEST_H__
#define __ARC_JOBLISTRETRIEVERPLUGINTEST_H__

#include <arc/compute/EntityRetriever.h>

namespace Arc {

class JobListRetrieverPluginTEST : public JobListRetrieverPlugin {
public:
  JobListRetrieverPluginTEST(PluginArgument* parg):
    JobListRetrieverPlugin(parg)
  {
    supportedInterfaces.push_back("org.nordugrid.jlrtest");
  }
  ~JobListRetrieverPluginTEST() {}

  static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginTEST(arg); }
  virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
  virtual bool isEndpointNotSupported(const Endpoint& endpoint) const { return endpoint.URLString.empty(); }
};

}

#endif // __ARC_JOBLISTRETRIEVERPLUGINTEST_H__
