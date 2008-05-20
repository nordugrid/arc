#include "TargetRetrieverARC0.h"
#include "SubmitterARC0.h"

acc_descriptors ARC_ACC_LOADER = {
  {"TargetRetrieverARC0", 0, &Arc::TargetRetrieverARC0::Instance},
  {"SubmitterARC0", 0, &Arc::SubmitterARC0::Instance},
  {NULL, 0, NULL}
};
