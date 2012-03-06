// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINLDAPNG_H__
#define __ARC_JOBLISTRETRIEVERPLUGINLDAPNG_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginLDAPNG : public EntityRetrieverPlugin<ComputingInfoEndpoint, Job> {
  public:
    JobListRetrieverPluginLDAPNG() { supportedInterfaces.push_back("org.nordugrid.ldapng"); }
    virtual ~JobListRetrieverPluginLDAPNG() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginLDAPNG(); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const ComputingInfoEndpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINLDAPNG_H__
