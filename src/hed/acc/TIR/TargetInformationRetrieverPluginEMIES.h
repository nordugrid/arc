// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVEREMIES_H__
#define __ARC_TARGETINFORMATIONRETRIEVEREMIES_H__

#include <list>

#include "TargetInformationRetriever.h"
#include "TargetInformationRetrieverPlugin.h"

namespace Arc {

  class Logger;
  class EndpointQueryingStatus;
  class ExecutionTarget;
  class URL;
  class UserConfig;
  class XMLNode;

  class TargetInformationRetrieverPluginEMIES: public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginEMIES() {};
    ~TargetInformationRetrieverPluginEMIES() {};
    static Plugin* Instance(PluginArgument *) { return new TargetInformationRetrieverPluginEMIES(); };

    EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<ExecutionTarget>&) const;
    static void ExtractTargets(const URL&, XMLNode response, std::list<ExecutionTarget>&);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVEREMIES_H__
