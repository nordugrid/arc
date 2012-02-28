// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
#define __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__

#include <arc/client/Job.h>
#include <arc/client/EndpointRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginEMIES : public EndpointRetrieverPlugin<ComputingInfoEndpoint, Job> {
  public:
    JobListRetrieverPluginEMIES() {}
    ~JobListRetrieverPluginEMIES() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginEMIES(); }
    EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
