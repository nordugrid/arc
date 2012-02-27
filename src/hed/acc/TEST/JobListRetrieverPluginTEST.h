#ifndef __ARC_JOBLISTRETRIEVERPLUGINTEST_H__
#define __ARC_JOBLISTRETRIEVERPLUGINTEST_H__

#include <arc/client/JobListRetriever.h>

namespace Arc {

class JobListRetrieverPluginTEST : public EndpointRetrieverPlugin<ComputingInfoEndpoint, Job> {
public:
  JobListRetrieverPluginTEST() { supportedInterfaces.push_back("org.nordugrid.jlrtest"); }
  ~JobListRetrieverPluginTEST() {}

  static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginTEST; }
  virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointFilter<Job>&) const;
};

}

#endif // __ARC_JOBLISTRETRIEVERPLUGINTEST_H__
