#ifndef __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__
#define __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__

#include <arc/compute/EntityRetriever.h>
#include <arc/loader/Plugin.h>

namespace Arc {

class TargetInformationRetrieverPluginTEST : public TargetInformationRetrieverPlugin {
protected:
  TargetInformationRetrieverPluginTEST(PluginArgument* parg):
    TargetInformationRetrieverPlugin(parg)
  {
    supportedInterfaces.push_back("org.nordugrid.tirtest");
  }
public:
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const Endpoint& registry,
                                       std::list<ComputingServiceType>& endpoints,
                                       const EndpointQueryOptions<ComputingServiceType>&) const;
  static Plugin* Instance(PluginArgument *arg);
  virtual bool isEndpointNotSupported(const Endpoint& endpoint) const { return endpoint.URLString.empty(); }
};

}

#endif // __ARC_TARGETINFORMATIONRETRIEVERPLUGINTEST_H__
