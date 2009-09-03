// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "FastestQueueBroker.h"
#include "RandomBroker.h"
#include "BenchmarkBroker.h"
#include "DataBroker.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "FastestQueue", "HED:Broker", 0, &Arc::FastestQueueBroker::Instance },
  { "Random", "HED:Broker", 0, &Arc::RandomBroker::Instance },
  { "Benchmark", "HED:Broker", 0, &Arc::BenchmarkBroker::Instance },
  { "Data", "HED:Broker", 0, &Arc::DataBroker::Instance },
  { NULL, NULL, 0, NULL }
};
