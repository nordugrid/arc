#include "BrokerTestACC.h"
#include "JobControllerTestACC.h"
#include "JobDescriptionParserTestACC.h"
#include "SubmitterTestACC.h"
#include "TargetInformationRetrieverPluginTEST.h"
#include "ServiceEndpointRetrieverPluginTEST.h"
#include "JobListRetrieverPluginTEST.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:Broker", "", 0, &Arc::BrokerTestACC::GetInstance },
  { "TEST", "HED:JobController", "", 0, &Arc::JobControllerTestACC::GetInstance },
  { "TEST", "HED:JobDescriptionParser", "", 0, &Arc::JobDescriptionParserTestACC::GetInstance },
  { "TEST", "HED:Submitter", "", 0, &Arc::SubmitterTestACC::GetInstance },
  { "TEST", Arc::TargetInformationRetrieverPluginTEST::kind.c_str(), "TargetInformationRetriever test plugin", 0, &Arc::TargetInformationRetrieverPluginTEST::Instance },
  { "TEST", Arc::ServiceEndpointRetrieverPluginTEST::kind.c_str(), "ServiceEndpointRetriever test plugin", 0, &Arc::ServiceEndpointRetrieverPluginTEST::Instance },
  { "TEST", Arc::JobListRetrieverPluginTEST::kind.c_str(), "JobListRetriever test plugin", 0, &Arc::JobListRetrieverPluginTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
