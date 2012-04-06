// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/loader/Plugin.h>

#include "JobListRetrieverPluginLDAPNG.h"
#include "TargetInformationRetrieverPluginLDAPGLUE1.h"
#include "TargetInformationRetrieverPluginLDAPGLUE2.h"
#include "TargetInformationRetrieverPluginLDAPNG.h"
#include "ServiceEndpointRetrieverPluginEGIIS.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "LDAPNG", "HED:JobListRetrieverPlugin", "", 0, &Arc::JobListRetrieverPluginLDAPNG::Instance },
  { "LDAPGLUE1", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginLDAPGLUE1::Instance },
  { "LDAPGLUE2", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginLDAPGLUE2::Instance },
  { "LDAPNG", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginLDAPNG::Instance },
  { "EGIIS", "HED:ServiceEndpointRetrieverPlugin", "", 0, &Arc::ServiceEndpointRetrieverPluginEGIIS::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
