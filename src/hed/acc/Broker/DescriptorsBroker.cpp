// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "FastestQueueBrokerPlugin.h"
#include "RandomBrokerPlugin.h"
#include "BenchmarkBrokerPlugin.h"
#include "DataBrokerPlugin.h"
#include "NullBrokerPlugin.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "FastestQueue", "HED:BrokerPlugin", istring("Sorting according to free slots in queue"), 0, &Arc::FastestQueueBrokerPlugin::Instance },
  { "Random", "HED:BrokerPlugin", istring("Random sorting"), 0, &Arc::RandomBrokerPlugin::Instance },
  { "Benchmark", "HED:BrokerPlugin", istring("Sorting according to specified benchmark (default \"specint2000\")"), 0, &Arc::BenchmarkBrokerPlugin::Instance },
  { "Data", "HED:BrokerPlugin", istring("Sorting according to input data availability at target"), 0, &Arc::DataBrokerPlugin::Instance },
  { "Null", "HED:BrokerPlugin", istring("Performs neither sorting nor matching"), 0, &Arc::NullBrokerPlugin::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
