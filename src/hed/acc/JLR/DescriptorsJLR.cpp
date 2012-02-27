// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/loader/Plugin.h>

#include "JobListRetrieverPluginLDAPNG.h"
#include "JobListRetrieverPluginEMIES.h"
#include "JobListRetrieverPluginWSRFBES.h"
#include "JobListRetrieverPluginWSRFCREAM.h"
#include "JobListRetrieverPluginWSRFGLUE2.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "WSRFCREAM", Arc::JobListRetrieverPlugin::kind.c_str(), "", 0, &Arc::JobListRetrieverPluginWSRFCREAM::Instance },
  { "LDAPNG", Arc::JobListRetrieverPlugin::kind.c_str(), "", 0, &Arc::JobListRetrieverPluginLDAPNG::Instance },
  { "WSRFBES", Arc::JobListRetrieverPlugin::kind.c_str(), "", 0, &Arc::JobListRetrieverPluginWSRFBES::Instance },
  { "WSRFGLUE2", Arc::JobListRetrieverPlugin::kind.c_str(), "", 0, &Arc::JobListRetrieverPluginWSRFGLUE2::Instance },
  { "EMIES", Arc::JobListRetrieverPlugin::kind.c_str(), "", 0, &Arc::JobListRetrieverPluginEMIES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
