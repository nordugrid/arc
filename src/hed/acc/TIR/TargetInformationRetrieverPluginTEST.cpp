// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>

#include "TargetInformationRetrieverPluginTEST.h"

namespace Arc {

Plugin* TargetInformationRetrieverPluginTEST::Instance(PluginArgument* arg) {
  return new TargetInformationRetrieverPluginTEST();
}

EndpointQueryingStatus TargetInformationRetrieverPluginTEST::Query(const UserConfig& userconfig,
                                                                   const ComputingInfoEndpoint& endpoint,
                                                                   std::list<ExecutionTarget>& etList) const {
  Glib::usleep(TargetInformationRetrieverPluginTESTControl::delay*1000000);
  etList = TargetInformationRetrieverPluginTESTControl::etList;
  return TargetInformationRetrieverPluginTESTControl::status;
};

}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:TargetInformationRetrieverPlugin", "TargetInformationRetriever test plugin", 0, &Arc::TargetInformationRetrieverPluginTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
