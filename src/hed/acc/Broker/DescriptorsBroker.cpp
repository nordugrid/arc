#include <arc/client/ACCLoader.h>

#include "FastestQueueBroker.h"
#include "RandomBroker.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[]  = {
  { "FastestQueueBroker", "HED:ACC", 0, &Arc::FastestQueueBroker::Instance },
  { "RandomBroker", "HED:ACC", 0, &Arc::RandomBroker::Instance },
  { NULL, NULL, 0, NULL }
};
