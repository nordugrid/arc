#include <arc/loader/ACCLoader.h>

#include "FastestQueueBroker.h"
#include "RandomBroker.h"

acc_descriptors ARC_ACC_LOADER = {
  { "FastestQueueBroker", 0, &Arc::FastestQueueBroker::Instance },
  { "RandomBroker", 0, &Arc::RandomBroker::Instance },
  { NULL, 0, NULL }
};
