// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/loader/Plugin.h>

#include "JobListRetrieverPluginLDAPNG.h"
#include "JobListRetrieverPluginWSRFBES.h"
#include "JobListRetrieverPluginWSRFGLUE2.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "LDAPNG", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginLDAPNG::Instance },
  { "WSRFBES", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginWSRFBES::Instance },
  { "WSRFGLUE2", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginWSRFGLUE2::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
