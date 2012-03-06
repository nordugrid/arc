// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINWSRFBES_H__
#define __ARC_JOBLISTRETRIEVERPLUGINWSRFBES_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginWSRFBES : public EntityRetrieverPlugin<ComputingInfoEndpoint, Job> {
  public:
    JobListRetrieverPluginWSRFBES() { supportedInterfaces.push_back("org.ogf.bes"); }
    virtual ~JobListRetrieverPluginWSRFBES() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginWSRFBES(); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const; // No implementation in cpp file -- returns EndpointQueryingStatus::FAILED.
    virtual bool isEndpointNotSupported(const ComputingInfoEndpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINWSRFBES_H__
