#include <arc/loader/ACCLoader.h>

#include "SubmitterCREAM.h"
#include "TargetRetrieverCREAM.h"
#include "JobControllerCREAM.h"

acc_descriptors ARC_ACC_LOADER = {
  { "SubmitterCREAM", 0, &Arc::SubmitterCREAM::Instance },
  { "TargetRetrieverCREAM", 0, &Arc::TargetRetrieverCREAM::Instance },
  { "JobControllerCREAM", 0, &Arc::JobControllerCREAM::Instance },
  { NULL, 0, NULL }
};
