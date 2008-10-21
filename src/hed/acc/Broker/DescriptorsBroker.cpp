#include <arc/loader/ACCLoader.h>

#include "QueueBalanceBroker.h"
#include "RandomBroker.h"

acc_descriptors ARC_ACC_LOADER = {
  { "QueueBalanceBroker", 0, &Arc::QueueBalanceBroker::Instance },
  { "RandomBroker", 0, &Arc::RandomBroker::Instance },
  { NULL, 0, NULL }
};
