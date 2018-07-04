// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBLISTRETRIEVERPLUGININTERNAL_H__
#define __ARC_JOBLISTRETRIEVERPLUGININTERNAL_H__

#include <arc/compute/Job.h>
#include <arc/compute/EntityRetriever.h>


using namespace Arc;
namespace Arc{
  class Logger;
}

namespace ARexINTERNAL {


  class JobLocalDescription;
  class INTERNALClient;
  class INTERNALClients;



  class JobListRetrieverPluginINTERNAL : public Arc::JobListRetrieverPlugin {
  public:
    JobListRetrieverPluginINTERNAL(PluginArgument* parg): JobListRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.internal");
    }
    virtual ~JobListRetrieverPluginINTERNAL() {}

    static Plugin* Instance(PluginArgument *arg) { return new JobListRetrieverPluginINTERNAL(arg); }
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBLISTRETRIEVERPLUGININTERNAL_H__
