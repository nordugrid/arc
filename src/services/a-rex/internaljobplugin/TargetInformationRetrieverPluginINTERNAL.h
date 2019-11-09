// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERINTERNAL_H__
#define __ARC_TARGETINFORMATIONRETRIEVERINTERNAL_H__

#include <list>

#include <arc/compute/EntityRetriever.h>


using namespace Arc;

namespace Arc{

  class Logger;
  class EndpointQueryingStatus;
  class ExecutionTarget;
  class URL;
  class UserConfig;
  class XMLNode;


}

namespace ARexINTERNAL {



  class INTERNALClient;
  class JobStateINTERNAL;

  class TargetInformationRetrieverPluginINTERNAL: public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginINTERNAL(PluginArgument* parg):
        TargetInformationRetrieverPlugin(parg) {
        supportedInterfaces.push_back("org.nordugrid.internal");
    };
    ~TargetInformationRetrieverPluginINTERNAL() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginINTERNAL(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERINTERNAL_H__
