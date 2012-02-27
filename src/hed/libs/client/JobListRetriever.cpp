// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/Job.h>

#include "JobListRetriever.h"

namespace Arc {

  template<>
  const std::string JobListRetrieverPlugin::kind("HED:JobListRetrieverPlugin");

  template<>
  Logger EndpointRetriever<ComputingInfoEndpoint, Job>::logger(Logger::getRootLogger(), "JobListRetriever");

  template<>
  Logger EndpointRetrieverPluginLoader<ComputingInfoEndpoint, Job>::logger(Logger::getRootLogger(), "JobListRetrieverPluginLoader");

} // namespace Arc
