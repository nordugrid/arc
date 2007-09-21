#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/loader/DMCLoader.h>
#include "DMCHTTP.h"
#include "DataPointHTTP.h"

namespace Arc {

  Logger DMCHTTP::logger(DMC::logger, "HTTP");

  DMCHTTP::DMCHTTP(Config *cfg) : DMC(cfg) {
    Register(this);
  }

  DMCHTTP::~DMCHTTP() {
    Unregister(this);
  }

  DMC* DMCHTTP::Instance(Arc::Config *cfg, Arc::ChainContext*) {
    return new DMCHTTP(cfg);
  }

  DataPoint* DMCHTTP::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "http" &&
        url.Protocol() != "https" &&
        url.Protocol() != "httpg") return NULL;
    return new DataPointHTTP(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  { "http", 0, &Arc::DMCHTTP::Instance },
  { NULL, 0, NULL }
};
