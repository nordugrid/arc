// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/loader/Plugin.h>

#include "JobListRetrieverPluginLDAPNG.h"
#include "JobListRetrieverPluginLDAPGLUE2.h"
#include "TargetInformationRetrieverPluginLDAPGLUE1.h"
#include "TargetInformationRetrieverPluginLDAPGLUE2.h"
#include "TargetInformationRetrieverPluginLDAPNG.h"
#include "ServiceEndpointRetrieverPluginEGIIS.h"
#include "ServiceEndpointRetrieverPluginBDII.h"

Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
  { "LDAPNG", "HED:JobListRetrieverPlugin", "Classic NorduGrid LDAP Job List", 0, &Arc::JobListRetrieverPluginLDAPNG::Instance },
  { "LDAPGLUE2", "HED:JobListRetrieverPlugin", "GLUE2 LDAP Job List", 0, &Arc::JobListRetrieverPluginLDAPGLUE2::Instance },
  { "LDAPGLUE1", "HED:TargetInformationRetrieverPlugin", "GLUE1.x LDAP Local Information", 0, &Arc::TargetInformationRetrieverPluginLDAPGLUE1::Instance },
  { "LDAPGLUE2", "HED:TargetInformationRetrieverPlugin", "GLUE2 LDAP Local Information", 0, &Arc::TargetInformationRetrieverPluginLDAPGLUE2::Instance },
  { "LDAPNG", "HED:TargetInformationRetrieverPlugin", "Classic NorduGrid LDAP Local Information", 0, &Arc::TargetInformationRetrieverPluginLDAPNG::Instance },
  { "EGIIS", "HED:ServiceEndpointRetrieverPlugin", "Classic NorduGrid EGIIS Registry", 0, &Arc::ServiceEndpointRetrieverPluginEGIIS::Instance },
  { "BDII", "HED:ServiceEndpointRetrieverPlugin", "BDII top and site", 0, &Arc::ServiceEndpointRetrieverPluginBDII::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
