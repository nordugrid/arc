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
  Logger EndpointRetriever<ComputingInfoEndpoint, ExecutionTarget>::logger(Logger::getRootLogger(), "TargetInformationRetriever");

  template<>
  Logger EndpointRetrieverPluginLoader<ComputingInfoEndpoint, ExecutionTarget>::logger(Logger::getRootLogger(), "TargetInformationRetrieverPluginLoader");

} // namespace Arc
