#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__

#include <arc/compute/EntityRetriever.h>

namespace Arc {

class ServiceEndpointRetrieverPluginTEST : public ServiceEndpointRetrieverPlugin {
protected:
  ServiceEndpointRetrieverPluginTEST(PluginArgument* parg):
    ServiceEndpointRetrieverPlugin(parg)
  {
     supportedInterfaces.push_back("org.nordugrid.sertest");
  }
public:
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const Endpoint& registry,
                                       std::list<Endpoint>& endpoints,
                                       const EndpointQueryOptions<Endpoint>& options) const;
  static Plugin* Instance(PluginArgument *arg);
  virtual bool isEndpointNotSupported(const Endpoint& endpoint) const { return endpoint.URLString.empty(); }
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
