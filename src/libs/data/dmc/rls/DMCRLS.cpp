#include "DMCRLS.h"
#include "DataPointRLS.h"
#include "common/Logger.h"
#include "common/URL.h"
#include "src/hed/libs/loader/DMCLoader.h"

extern "C" {
#include "globus_rls_client.h"
}

namespace Arc {

  Logger DMCRLS::logger(DMC::logger, "RLS");

  DMCRLS::DMCRLS(Config *cfg) : DMC(cfg) {
    globus_module_activate(GLOBUS_COMMON_MODULE);
    globus_module_activate(GLOBUS_IO_MODULE);
    globus_module_activate(GLOBUS_RLS_CLIENT_MODULE);
    Register(this);
  }

  DMCRLS::~DMCRLS() {
    Unregister(this);
    globus_module_deactivate(GLOBUS_RLS_CLIENT_MODULE);
    globus_module_deactivate(GLOBUS_IO_MODULE);
    globus_module_deactivate(GLOBUS_COMMON_MODULE);
  }

  DMC* DMCRLS::Instance(Arc::Config *cfg, Arc::ChainContext *ctx) {
    return new DMCRLS(cfg);
  }

  DataPoint* DMCRLS::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "rls") return NULL;
    return new DataPointRLS(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  { "rls", 0, &Arc::DMCRLS::Instance },
  { NULL, 0, NULL }
};
