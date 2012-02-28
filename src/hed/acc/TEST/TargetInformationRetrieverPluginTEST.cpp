// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TestACCControl.h>

#include "TargetInformationRetrieverPluginTEST.h"

namespace Arc {

Plugin* TargetInformationRetrieverPluginTEST::Instance(PluginArgument* arg) {
  return new TargetInformationRetrieverPluginTEST();
}

EndpointQueryingStatus TargetInformationRetrieverPluginTEST::Query(const UserConfig& userconfig,
                                                                   const ComputingInfoEndpoint& endpoint,
                                                                   std::list<ExecutionTarget>& etList,
                                                                   const EndpointQueryOptions<ExecutionTarget>&) const {
  Glib::usleep(TargetInformationRetrieverPluginTESTControl::delay*1000000);
  etList = TargetInformationRetrieverPluginTESTControl::targets;
  return TargetInformationRetrieverPluginTESTControl::status;
};

}
