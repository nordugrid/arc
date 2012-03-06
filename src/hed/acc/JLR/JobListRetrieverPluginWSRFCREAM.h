// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginWSRFCREAM : public EntityRetrieverPlugin<ComputingInfoEndpoint, Job> {
  public:
    JobListRetrieverPluginWSRFCREAM() { supportedInterfaces.push_back("org.glite.wsrfcream"); }
    virtual ~JobListRetrieverPluginWSRFCREAM() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginWSRFCREAM(); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const; // No implementation in cpp file -- returns EndpointQueryingStatus::FAILED.
    virtual bool isEndpointNotSupported(const ComputingInfoEndpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
