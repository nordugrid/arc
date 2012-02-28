#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__

#include <arc/client/EndpointRetriever.h>

namespace Arc {

class ServiceEndpointRetrieverPluginTEST : public ServiceEndpointRetrieverPlugin {
protected:
  ServiceEndpointRetrieverPluginTEST() { supportedInterfaces.push_back("org.nordugrid.sertest"); }
public:
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const RegistryEndpoint& registry,
                                       std::list<ServiceEndpoint>& endpoints,
                                       const EndpointQueryOptions<ServiceEndpoint>& options) const;
  static Plugin* Instance(PluginArgument *arg);
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
