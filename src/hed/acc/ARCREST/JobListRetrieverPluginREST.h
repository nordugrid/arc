// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINREST_H__
#define __ARC_JOBLISTRETRIEVERPLUGINREST_H__

#include <arc/compute/Job.h>
#include <arc/compute/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginREST : public JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginREST(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.arcrest");
    }
    virtual ~JobListRetrieverPluginREST() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginREST(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINREST_H__
