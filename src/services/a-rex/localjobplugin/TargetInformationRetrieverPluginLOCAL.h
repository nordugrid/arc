// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERLOCAL_H__
#define __ARC_TARGETINFORMATIONRETRIEVERLOCAL_H__

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

namespace ARexLOCAL {



  class LOCALClient;
  class JobStateLOCAL;

  class TargetInformationRetrieverPluginLOCAL: public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginLOCAL(PluginArgument* parg):
        TargetInformationRetrieverPlugin(parg) {
        supportedInterfaces.push_back("org.nordugrid.internal");
    };
    ~TargetInformationRetrieverPluginLOCAL() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginLOCAL(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERLOCAL_H__
