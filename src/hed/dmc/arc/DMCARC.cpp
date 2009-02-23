#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>

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

  DMC *DMCARC::Instance(Config *cfg) {
    return new DMCARC(cfg);
  }

  DataPoint *DMCARC::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "arc")
      return NULL;
    return new DataPointARC(url);
  }

} // namespace Arc
