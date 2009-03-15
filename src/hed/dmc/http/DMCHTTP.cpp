// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMCLoader.h>

#include "DataPointHTTP.h"
#include "DMCHTTP.h"

namespace Arc {

  Logger DMCHTTP::logger(DMC::logger, "HTTP");

  DMCHTTP::DMCHTTP(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCHTTP::~DMCHTTP() {
    Unregister(this);
  }

  Plugin* DMCHTTP::Instance(PluginArgument *arg) {
    Arc::DMCPluginArgument *dmcarg =
      arg ? dynamic_cast<Arc::DMCPluginArgument*>(arg) : NULL;
    if (!dmcarg)
      return NULL;
    return new DMCHTTP((Arc::Config*)(*dmcarg));
  }

  DataPoint* DMCHTTP::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "http" &&
        url.Protocol() != "https" &&
        url.Protocol() != "httpg")
      return NULL;
    return new DataPointHTTP(url);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "http", "HED:DMC", 0, &Arc::DMCHTTP::Instance },
  { NULL, NULL, 0, NULL }
};
