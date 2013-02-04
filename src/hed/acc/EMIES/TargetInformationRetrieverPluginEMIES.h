// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVEREMIES_H__
#define __ARC_TARGETINFORMATIONRETRIEVEREMIES_H__

#include <list>

#include <arc/compute/EntityRetriever.h>

namespace Arc {

  class Logger;
  class EndpointQueryingStatus;
  class ExecutionTarget;
  class URL;
  class UserConfig;
  class XMLNode;

  class TargetInformationRetrieverPluginEMIES: public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginEMIES(PluginArgument* parg):
        TargetInformationRetrieverPlugin(parg) {
        supportedInterfaces.push_back("org.ogf.glue.emies.resourceinfo");
    };
    ~TargetInformationRetrieverPluginEMIES() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginEMIES(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;
    static void ExtractTargets(const URL&, XMLNode response, std::list<ComputingServiceType>&);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVEREMIES_H__
