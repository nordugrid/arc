// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>

#include "TargetInformationRetriever.h"

namespace Arc {

  float TargetInformationRetrieverPluginTESTControl::delay = 0;
  std::list<ExecutionTarget> TargetInformationRetrieverPluginTESTControl::etList;
  EndpointQueryingStatus TargetInformationRetrieverPluginTESTControl::status;

} // namespace Arc
