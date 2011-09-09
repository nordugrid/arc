// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "FastestQueueBroker.h"
#include "RandomBroker.h"
#include "BenchmarkBroker.h"
#include "DataBroker.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "FastestQueue", "HED:Broker", istring("Sorting according to free slots in queue"), 0, &Arc::FastestQueueBroker::Instance },
  { "Random", "HED:Broker", istring("Random sorting"), 0, &Arc::RandomBroker::Instance },
  { "Benchmark", "HED:Broker", istring("Sorting according to specified benchmark (default \"specint2000\")"), 0, &Arc::BenchmarkBroker::Instance },
  { "Data", "HED:Broker", istring("Sorting according to input data availability at target"), 0, &Arc::DataBroker::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
