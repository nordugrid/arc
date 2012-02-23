#ifndef __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__
#define __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__

#include <arc/client/EndpointRetriever.h>
#include <arc/loader/Plugin.h>

#include <arc/client/TargetInformationRetriever.h>

namespace Arc {

class TargetInformationRetrieverPluginTEST : public TargetInformationRetrieverPlugin {
protected:
  TargetInformationRetrieverPluginTEST() { supportedInterfaces.push_back("org.nordugrid.tirtest"); }
public:
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const ComputingInfoEndpoint& registry,
                                       std::list<ExecutionTarget>& endpoints,
                                       const EndpointFilter<ExecutionTarget>&) const;
  static Plugin* Instance(PluginArgument *arg);
};

}

#endif // __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__
