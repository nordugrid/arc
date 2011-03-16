#include "BrokerTestACC.h"
#include "JobControllerTestACC.h"
#include "JobDescriptionParserTestACC.h"
#include "SubmitterTestACC.h"
#include "TargetRetrieverTestACC.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:Broker", 0, &BrokerTestACC::GetInstance },
  { "TEST", "HED:JobController", 0, &JobControllerTestACC::GetInstance },
  { "TEST", "HED:JobDescriptionParser", 0, &JobDescriptionParserTestACC::GetInstance },
  { "TEST", "HED:Submitter", 0, &SubmitterTestACC::GetInstance },
  { "TEST", "HED:TargetRetriever", 0, &TargetRetrieverTestACC::GetInstance },
  { NULL, NULL, 0, NULL }
};
