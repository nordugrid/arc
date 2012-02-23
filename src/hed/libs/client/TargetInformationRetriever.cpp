// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>

#include "TargetInformationRetriever.h"

namespace Arc {

  template<>
  const std::string TargetInformationRetrieverPlugin::kind("HED:TargetInformationRetrieverPlugin");

  template<>
  Logger EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget>::logger(Logger::getRootLogger(), "TargetInformationRetriever");

  template<>
  Logger EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget>::logger(Logger::getRootLogger(), "TargetInformationRetriever");

  const std::string ComputingInfoEndpoint::ComputingInfoCapability = "information.discovery.resource";

  float TargetInformationRetrieverPluginTESTControl::delay = 0;
  std::list<ExecutionTarget> TargetInformationRetrieverPluginTESTControl::targets;
  EndpointQueryingStatus TargetInformationRetrieverPluginTESTControl::status;

} // namespace Arc
