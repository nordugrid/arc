#include <arc/loader/ACCLoader.h>

#include "TargetRetrieverUNICORE.h"
#include "JobControllerUNICORE.h"
#include "SubmitterUNICORE.h"

acc_descriptors ARC_ACC_LOADER = {
  { "TargetRetrieverUNICORE", 0, &Arc::TargetRetrieverUNICORE::Instance },
  { "SubmitterUNICORE", 0, &Arc::SubmitterUNICORE::Instance },
  { "JobControllerUNICORE", 0, &Arc::JobControllerUNICORE::Instance },
  { NULL, 0, NULL }
};
