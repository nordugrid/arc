#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/loader/DMCLoader.h>

#include "DataPointSRM.h"
#include "DMCSRM.h"

namespace Arc {

  Logger DMCSRM::logger(DMC::logger, "SRM");

  DMCSRM::DMCSRM(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCSRM::~DMCSRM() {
    Unregister(this);
  }

  DMC *DMCSRM::Instance(Config *cfg, ChainContext *) {
    return new DMCSRM(cfg);
  }

  DataPoint *DMCSRM::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "srm")
      return NULL;
    return new DataPointSRM(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  {"srm", 0, &Arc::DMCSRM::Instance},
  {NULL, 0, NULL}
};
