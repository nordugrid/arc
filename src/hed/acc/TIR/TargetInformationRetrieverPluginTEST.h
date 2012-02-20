#ifndef __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__
#define __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__

#include <arc/loader/Plugin.h>

#include "TargetInformationRetriever.h"
#include "TargetInformationRetrieverPlugin.h"

namespace Arc {

class TargetInformationRetrieverPluginTEST : public TargetInformationRetrieverPlugin {
protected:
  TargetInformationRetrieverPluginTEST() { supportedInterfaces.push_back("org.nordugrid.tirtest"); }
public:
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const ComputingInfoEndpoint& registry,
                                       std::list<ExecutionTarget>& endpoints) const;
  static Plugin* Instance(PluginArgument *arg);
};

}

#endif // __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__
