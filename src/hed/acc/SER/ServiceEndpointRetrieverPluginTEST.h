#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__

#include <arc/loader/Plugin.h>

#include "ServiceEndpointRetriever.h"
#include "ServiceEndpointRetrieverPlugin.h"

namespace Arc {

class ServiceEndpointRetrieverTEST : public ServiceEndpointRetrieverPlugin {
protected:
  ServiceEndpointRetrieverTEST() { supportedInterface.push_back("org.nordugrid.sertest"); }
public:
  virtual RegistryEndpointStatus Query(const UserConfig& userconfig,
                                       const RegistryEndpoint& registry,
                                       std::list<ServiceEndpoint>& endpoints) const;
  static Plugin* Instance(PluginArgument *arg);
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
