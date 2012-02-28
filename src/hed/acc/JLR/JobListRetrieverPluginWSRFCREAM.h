// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__

#include <arc/client/Job.h>
#include <arc/client/EndpointRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginWSRFCREAM : public EndpointRetrieverPlugin<ComputingInfoEndpoint, Job> {
  public:
    JobListRetrieverPluginWSRFCREAM() {}
    ~JobListRetrieverPluginWSRFCREAM() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginWSRFCREAM(); }
    EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const; // No implementation in cpp file -- returns EndpointQueryingStatus::FAILED.

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
