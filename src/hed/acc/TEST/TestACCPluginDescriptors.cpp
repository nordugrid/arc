#include "BrokerTestACC.h"
#include "JobControllerTestACC.h"
#include "JobDescriptionParserTestACC.h"
#include "SubmitterTestACC.h"
#include "TargetRetrieverTestACC.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:Broker", "", 0, &Arc::BrokerTestACC::GetInstance },
  { "TEST", "HED:JobController", "", 0, &Arc::JobControllerTestACC::GetInstance },
  { "TEST", "HED:JobDescriptionParser", "", 0, &Arc::JobDescriptionParserTestACC::GetInstance },
  { "TEST", "HED:Submitter", "", 0, &Arc::SubmitterTestACC::GetInstance },
  { "TEST", "HED:TargetRetriever", "", 0, &Arc::TargetRetrieverTestACC::GetInstance },
  { NULL, NULL, NULL, 0, NULL }
};
