#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMCLoader.h>

#include "DataPointLFC.h"
#include "DMCLFC.h"

namespace Arc {

  Logger DMCLFC::logger(DMC::logger, "LFC");

  DMCLFC::DMCLFC(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCLFC::~DMCLFC() {
    Unregister(this);
  }

  Plugin *DMCLFC::Instance(PluginArgunent* arg) {
    Arc::DMCPluginArgument* dmcarg =
            arg?dynamic_cast<Arc::DMCPluginArgument*>(arg):NULL;
    if(!dmcarg) return NULL;
    return new DMCLFC((Arc::Config*)(*dmcarg));
  }

  DataPoint *DMCLFC::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "lfc")
      return NULL;
    return new DataPointLFC(url);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  {"lfc", "HED:DMC", 0, &Arc::DMCLFC::Instance},
  {NULL, NULL, 0, NULL}
};
