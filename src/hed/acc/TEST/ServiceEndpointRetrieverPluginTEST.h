#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__

#include <arc/client/EntityRetriever.h>

namespace Arc {

class ServiceEndpointRetrieverPluginTEST : public ServiceEndpointRetrieverPlugin {
protected:
  ServiceEndpointRetrieverPluginTEST() { supportedInterfaces.push_back("org.nordugrid.sertest"); }
public:
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const Endpoint& registry,
                                       std::list<ServiceEndpoint>& endpoints,
                                       const EndpointQueryOptions<ServiceEndpoint>& options) const;
  static Plugin* Instance(PluginArgument *arg);
  virtual bool isEndpointNotSupported(const Endpoint& endpoint) const { return endpoint.URLString.empty(); }
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
