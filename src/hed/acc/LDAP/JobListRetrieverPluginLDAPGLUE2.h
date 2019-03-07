// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINLDAPGLUE2_H__
#define __ARC_JOBLISTRETRIEVERPLUGINLDAPGLUE2_H__

#include <arc/compute/Job.h>
#include <arc/compute/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginLDAPGLUE2 : public JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginLDAPGLUE2(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.ldapglue2");
    }
    virtual ~JobListRetrieverPluginLDAPGLUE2() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginLDAPGLUE2(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINLDAPGLUE2_H__
