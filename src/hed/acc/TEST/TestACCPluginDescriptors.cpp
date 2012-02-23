#include "BrokerTestACC.h"
#include "JobControllerTestACC.h"
#include "JobDescriptionParserTestACC.h"
#include "SubmitterTestACC.h"
#include "TargetRetrieverTestACC.h"
#include "TargetInformationRetrieverPluginTEST.h"
#include "ServiceEndpointRetrieverPluginTEST.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:Broker", "", 0, &Arc::BrokerTestACC::GetInstance },
  { "TEST", "HED:JobController", "", 0, &Arc::JobControllerTestACC::GetInstance },
  { "TEST", "HED:JobDescriptionParser", "", 0, &Arc::JobDescriptionParserTestACC::GetInstance },
  { "TEST", "HED:Submitter", "", 0, &Arc::SubmitterTestACC::GetInstance },
  { "TEST", "HED:TargetRetriever", "", 0, &Arc::TargetRetrieverTestACC::GetInstance },
  { "TEST", "HED:TargetInformationRetrieverPlugin", "TargetInformationRetriever test plugin", 0, &Arc::TargetInformationRetrieverPluginTEST::Instance },
  { "TEST", "HED:ServiceEndpointRetrieverPlugin", "ServiceEndpointRetriever test plugin", 0, &Arc::ServiceEndpointRetrieverTEST::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
