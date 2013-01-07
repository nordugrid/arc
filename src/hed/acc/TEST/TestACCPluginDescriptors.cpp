#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "BrokerPluginTestACC.h"
#include "JobControllerPluginTestACC.h"
#include "JobDescriptionParserPluginTestACC.h"
#include "SubmitterPluginTestACC.h"
#include "TargetInformationRetrieverPluginTEST.h"
#include "ServiceEndpointRetrieverPluginTEST.h"
#include "JobListRetrieverPluginTEST.h"

Arc::PluginDescriptor ARC_PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:BrokerPlugin", "", 0, &Arc::BrokerPluginTestACC::GetInstance },
  { "TEST", "HED:JobControllerPlugin", "", 0, &Arc::JobControllerPluginTestACC::GetInstance },
  { "TEST", "HED:JobDescriptionParserPlugin", "", 0, &Arc::JobDescriptionParserPluginTestACC::GetInstance },
  { "TEST", "HED:SubmitterPlugin", "", 0, &Arc::SubmitterPluginTestACC::GetInstance },
  { "TEST", Arc::TargetInformationRetrieverPluginTEST::kind.c_str(), "TargetInformationRetriever test plugin", 0, &Arc::TargetInformationRetrieverPluginTEST::Instance },
  { "TEST", Arc::ServiceEndpointRetrieverPluginTEST::kind.c_str(), "ServiceEndpointRetriever test plugin", 0, &Arc::ServiceEndpointRetrieverPluginTEST::Instance },
  { "TEST", Arc::JobListRetrieverPluginTEST::kind.c_str(), "JobListRetriever test plugin", 0, &Arc::JobListRetrieverPluginTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
