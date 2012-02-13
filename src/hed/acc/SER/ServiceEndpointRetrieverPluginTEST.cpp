// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ServiceEndpointRetrieverPluginTEST.h"

namespace Arc {

Plugin* ServiceEndpointRetrieverTEST::Instance(PluginArgument* arg) {
  return new ServiceEndpointRetrieverTEST();
}

RegistryEndpointStatus ServiceEndpointRetrieverTEST::Query(const UserConfig& userconfig,
                                                          const RegistryEndpoint& endpoint,
                                                          ServiceEndpointConsumer& consumer) {
  Glib::usleep(ServiceEndpointRetrieverTESTControl::delay*1000000);
  for (std::list<ServiceEndpoint>::const_iterator it = ServiceEndpointRetrieverTESTControl::endpoints.begin();
       it != ServiceEndpointRetrieverTESTControl::endpoints.end(); it++) {
    consumer.addServiceEndpoint(*it);
  }
  return ServiceEndpointRetrieverTESTControl::status;
};


}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:ServiceEndpointRetrieverPlugin", "ServiceEndpointRetriever test plugin", 0, &Arc::ServiceEndpointRetrieverTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
