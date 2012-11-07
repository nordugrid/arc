// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINLDAPNG_H__
#define __ARC_JOBLISTRETRIEVERPLUGINLDAPNG_H__

#include <arc/compute/Job.h>
#include <arc/compute/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginLDAPNG : public JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginLDAPNG(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.ldapng");
    }
    virtual ~JobListRetrieverPluginLDAPNG() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginLDAPNG(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINLDAPNG_H__
