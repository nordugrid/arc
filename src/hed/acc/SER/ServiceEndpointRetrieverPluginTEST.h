#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__

#include <arc/loader/Plugin.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

class ServiceEndpointRetrieverTEST : public ServiceEndpointRetrieverPlugin {
protected:
  ServiceEndpointRetrieverTEST() : testTimeout(*ServiceEndpointRetrieverTESTControl::tcTimeout), testStatus(*ServiceEndpointRetrieverTESTControl::tcStatus) {}
public:
  virtual ServiceEndpointStatus Query(const UserConfig& uc, const RegistryEndpoint& rEndpoint, ServiceEndpointConsumer& consumer)
    {
      sleep(testTimeout);
      return testStatus;
    };

  static Plugin* Instance(PluginArgument *arg);

private:
  const int& testTimeout;
  const ServiceEndpointStatus& testStatus;
};

}

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGINTEST_H__
