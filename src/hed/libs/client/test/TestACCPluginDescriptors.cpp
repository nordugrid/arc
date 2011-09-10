#include "BrokerTestACC.h"
#include "JobControllerTestACC.h"
#include "JobDescriptionParserTestACC.h"
#include "SubmitterTestACC.h"
#include "TargetRetrieverTestACC.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "TEST", "HED:Broker", NULL, 0, &BrokerTestACC::GetInstance },
  { "TEST", "HED:JobController", NULL, 0, &JobControllerTestACC::GetInstance },
  { "TEST", "HED:JobDescriptionParser", NULL, 0, &JobDescriptionParserTestACC::GetInstance },
  { "TEST", "HED:Submitter", NULL, 0, &SubmitterTestACC::GetInstance },
  { "TEST", "HED:TargetRetriever", NULL, 0, &TargetRetrieverTestACC::GetInstance },
  { NULL, NULL, 0, NULL }
};
