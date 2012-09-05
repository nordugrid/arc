// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginARC1 : public JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginARC1(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.wsrfglue2");
    }
    virtual ~JobListRetrieverPluginARC1() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginARC1(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__
