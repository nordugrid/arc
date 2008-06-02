#include <arc/loader/ACCLoader.h>

#include "TargetRetrieverARC1.h"
#include "JobControllerARC1.h"
#include "SubmitterARC1.h"

acc_descriptors ARC_ACC_LOADER = {
  {"TargetRetrieverARC1", 0, &Arc::TargetRetrieverARC1::Instance},
  {"SubmitterARC1", 0, &Arc::SubmitterARC1::Instance},
  {"JobControllerARC1", 0, &Arc::JobControllerARC1::Instance},
  {NULL, 0, NULL}
};
