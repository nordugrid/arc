// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
#define __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__

#include <arc/compute/Job.h>
#include <arc/compute/EntityRetriever.h>
#include <arc/compute/WSCommonPlugin.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginEMIES : public WSCommonPlugin<JobListRetrieverPlugin> {
  public:
    JobListRetrieverPluginEMIES(PluginArgument* parg): WSCommonPlugin<JobListRetrieverPlugin>(parg) {
      supportedInterfaces.push_back("org.ogf.glue.emies.resourceinfo");
    }
    virtual ~JobListRetrieverPluginEMIES() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginEMIES(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
