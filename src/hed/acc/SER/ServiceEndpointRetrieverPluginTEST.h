#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__

#include <arc/loader/Plugin.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

class ServiceEndpointRetrieverTEST : public ServiceEndpointRetrieverPlugin {
protected:
  ServiceEndpointRetrieverTEST() {}
public:
  virtual RegistryEndpointStatus Query(const UserConfig& userconfig,
                                       const RegistryEndpoint& registry,
                                       std::list<ServiceEndpoint>& endpoints);
  static Plugin* Instance(PluginArgument *arg);
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
