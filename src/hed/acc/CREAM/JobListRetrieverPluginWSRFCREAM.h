// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginWSRFCREAM : public JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginWSRFCREAM(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.glite.wsrfcream");
    }
    virtual ~JobListRetrieverPluginWSRFCREAM() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginWSRFCREAM(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const; // No implementation in cpp file -- returns EndpointQueryingStatus::FAILED.
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFCREAM_H__
