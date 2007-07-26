#include "DMCLFC.h"
#include "DataPointLFC.h"
#include "common/Logger.h"
#include "common/URL.h"
#include "loader/DMCLoader.h"

namespace Arc {

  Logger DMCLFC::logger(DMC::logger, "LFC");

  DMCLFC::DMCLFC(Config *cfg) : DMC(cfg) {
    Register(this);
  }

  DMCLFC::~DMCLFC() {
    Unregister(this);
  }

  DMC* DMCLFC::Instance(Arc::Config *cfg, Arc::ChainContext *ctx) {
    return new DMCLFC(cfg);
  }

  DataPoint* DMCLFC::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "lfc") return NULL;
    return new DataPointLFC(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  { "lfc", 0, &Arc::DMCLFC::Instance },
  { NULL, 0, NULL }
};
