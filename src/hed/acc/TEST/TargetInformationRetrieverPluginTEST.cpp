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
  return new TargetInformationRetrieverPluginTEST(arg);
}

EndpointQueryingStatus TargetInformationRetrieverPluginTEST::Query(const UserConfig& userconfig,
                                                                   const Endpoint& endpoint,
                                                                   std::list<ComputingServiceType>& csList,
                                                                   const EndpointQueryOptions<ComputingServiceType>&) const {
  Glib::usleep(TargetInformationRetrieverPluginTESTControl::delay*1000000);
  csList = TargetInformationRetrieverPluginTESTControl::targets;
  return TargetInformationRetrieverPluginTESTControl::status;
};

}
