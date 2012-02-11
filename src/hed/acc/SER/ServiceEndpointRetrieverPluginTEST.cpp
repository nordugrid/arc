// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ServiceEndpointRetrieverPluginTEST.h"

namespace Arc {

Plugin* ServiceEndpointRetrieverTEST::Instance(PluginArgument* arg) {
  return new ServiceEndpointRetrieverTEST();
}

RegistryEndpointStatus ServiceEndpointRetrieverTEST::Query(const UserConfig& uc,
                                                          const RegistryEndpoint& rEndpoint,
                                                          ServiceEndpointConsumer& consumer) {
  for (std::list<ServiceEndpoint>::const_iterator it = ServiceEndpointRetrieverTESTControl::tcEndpoints.begin();
       it != ServiceEndpointRetrieverTESTControl::tcEndpoints.end(); it++) {
    Glib::usleep(ServiceEndpointRetrieverTESTControl::tcPeriod*1000000);
    consumer.addServiceEndpoint(*it);
  }
  return ServiceEndpointRetrieverTESTControl::tcStatus;
};


}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:ServiceEndpointRetrieverPlugin", "ServiceEndpointRetriever test plugin", 0, &Arc::ServiceEndpointRetrieverTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
