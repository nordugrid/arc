// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__

#include <arc/compute/Job.h>
#include <arc/compute/EntityRetriever.h>
#include <arc/compute/WSCommonPlugin.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginWSRFCREAM : public WSCommonPlugin<JobListRetrieverPlugin> {
  public:
    JobListRetrieverPluginWSRFCREAM(PluginArgument* parg): WSCommonPlugin<JobListRetrieverPlugin>(parg) {
      supportedInterfaces.push_back("org.glite.cream");
      supportedInterfaces.push_back("org.glite.ce.cream");
    }
    virtual ~JobListRetrieverPluginWSRFCREAM() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginWSRFCREAM(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const; // No implementation in cpp file -- returns EndpointQueryingStatus::FAILED.

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
