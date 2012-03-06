// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginWSRFGLUE2 : public EntityRetrieverPlugin<ComputingInfoEndpoint, Job> {
  public:
    JobListRetrieverPluginWSRFGLUE2() { supportedInterfaces.push_back("org.nordugrid.wsrfglue2"); }
    virtual ~JobListRetrieverPluginWSRFGLUE2() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginWSRFGLUE2(); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const ComputingInfoEndpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFGLUE2_H__
