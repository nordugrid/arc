// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "FastestQueueBrokerPlugin.h"
#include "RandomBrokerPlugin.h"
#include "BenchmarkBrokerPlugin.h"
#include "DataBrokerPlugin.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "FastestQueue", "HED:BrokerPlugin", istring("Sorting according to free slots in queue"), 0, &Arc::FastestQueueBrokerPlugin::Instance },
  { "Random", "HED:BrokerPlugin", istring("Random sorting"), 0, &Arc::RandomBrokerPlugin::Instance },
  { "Benchmark", "HED:BrokerPlugin", istring("Sorting according to specified benchmark (default \"specint2000\")"), 0, &Arc::BenchmarkBrokerPlugin::Instance },
  { "Data", "HED:BrokerPlugin", istring("Sorting according to input data availability at target"), 0, &Arc::DataBrokerPlugin::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
