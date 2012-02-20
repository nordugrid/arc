// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/EndpointQueryingStatus.h>
#include <arc/loader/Plugin.h>

#include "TargetInformationRetrieverPluginLDAPGLUE1.h"
#include "TargetInformationRetrieverPluginLDAPNG.h"
#include "TargetInformationRetrieverPluginBES.h"
#include "TargetInformationRetrieverPluginWSRFGLUE2.h"
#include "TargetInformationRetrieverPluginEMIES.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "LDAPGLUE1", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginLDAPGLUE1::Instance },
  { "LDAPNG", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginLDAPNG::Instance },
  { "BES", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginBES::Instance },
  { "WSRFGLUE2", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginWSRFGLUE2::Instance },
  { "EMIES", "HED:TargetInformationRetrieverPlugin", "", 0, &Arc::TargetInformationRetrieverPluginEMIES::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
