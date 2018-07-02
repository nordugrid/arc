// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGINLOCAL_H__
#define __ARC_JOBLISTRETRIEVERPLUGINLOCAL_H__

#include <arc/compute/Job.h>
#include <arc/compute/EntityRetriever.h>


using namespace Arc;
namespace Arc{
  class Logger;
}

namespace ARexLOCAL {


  class JobLocalDescription;
  class LOCALClient;
  class LOCALClients;



  class JobListRetrieverPluginLOCAL : public Arc::JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginLOCAL(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.internal");
    }
    virtual ~JobListRetrieverPluginLOCAL() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginLOCAL(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGINLOCAL_H__
