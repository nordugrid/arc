#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__

#include <arc/client/ServiceEndpointRetriever.h>

namespace Arc {

class ServiceEndpointRetrieverPluginTEST : public EndpointRetrieverPlugin<RegistryEndpoint, ServiceEndpoint> {
protected:
  ServiceEndpointRetrieverPluginTEST() { supportedInterfaces.push_back("org.nordugrid.sertest"); }
public:
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const RegistryEndpoint& registry,
                                       std::list<ServiceEndpoint>& endpoints,
                                       const EndpointFilter<ServiceEndpoint>& filter) const;
  static Plugin* Instance(PluginArgument *arg);
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
