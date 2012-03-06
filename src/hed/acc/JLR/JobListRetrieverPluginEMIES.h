// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
#define __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginEMIES : public EntityRetrieverPlugin<ComputingInfoEndpoint, Job> {
  public:
    JobListRetrieverPluginEMIES() { supportedInterfaces.push_back("org.ogf.emies"); }
    virtual ~JobListRetrieverPluginEMIES() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginEMIES(); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const ComputingInfoEndpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
