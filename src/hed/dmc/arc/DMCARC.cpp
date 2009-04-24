// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMCLoader.h>

#include "DataPointARC.h"
#include "DMCARC.h"

namespace Arc {

  Logger DMCARC::logger(DMC::logger, "ARC");

  DMCARC::DMCARC(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCARC::~DMCARC() {
    Unregister(this);
  }

  Plugin* DMCARC::Instance(PluginArgument *arg) {
    DMCPluginArgument *dmcarg =
      arg ? dynamic_cast<DMCPluginArgument*>(arg) : NULL;
    if (!dmcarg)
      return NULL;
    return new DMCARC((Config*)(*dmcarg));
  }

  DataPoint* DMCARC::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "arc")
      return NULL;
    return new DataPointARC(url);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "arc", "HED:DMC", 0, &Arc::DMCARC::Instance },
  { NULL, NULL, 0, NULL }
};
