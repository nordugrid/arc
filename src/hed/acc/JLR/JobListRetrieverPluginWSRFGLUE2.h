// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginWSRFGLUE2 : public JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginWSRFGLUE2(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.wsrfglue2");
    }
    virtual ~JobListRetrieverPluginWSRFGLUE2() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginWSRFGLUE2(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__
