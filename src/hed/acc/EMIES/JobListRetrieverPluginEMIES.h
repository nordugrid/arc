// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
#define __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__

#include <arc/client/Job.h>
#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class JobListRetrieverPluginEMIES : public JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginEMIES(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.ogf.glue.emies.activityinfo");
      supportedInterfaces.push_back("org.ogf.glue.emies.activitymanagement");
      supportedInterfaces.push_back("org.ogf.emies");
    }
    virtual ~JobListRetrieverPluginEMIES() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginEMIES(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINEMIES_H__
