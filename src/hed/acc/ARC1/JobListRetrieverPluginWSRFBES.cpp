// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "JobListRetrieverPluginWSRFBES.h"

namespace Arc {

  Logger JobListRetrieverPluginWSRFBES::logger(Logger::getRootLogger(), "JobListRetrieverPlugin.WSRFBES");

  EndpointQueryingStatus JobListRetrieverPluginWSRFBES::Query(const UserConfig&, const Endpoint&, std::list<Job>&, const EndpointQueryOptions<Job>&) const {
    return EndpointQueryingStatus::FAILED;
  }

} // namespace Arc

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "WSRFBES", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginWSRFBES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
