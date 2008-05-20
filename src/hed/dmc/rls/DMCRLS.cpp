#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

extern "C" {
#include "globus_rls_client.h"
}

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/loader/DMCLoader.h>

#include "DataPointRLS.h"
#include "DMCRLS.h"

namespace Arc {

  Logger DMCRLS::logger(DMC::logger, "RLS");

  DMCRLS::DMCRLS(Config *cfg)
    : DMC(cfg) {
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

  DMC *DMCRLS::Instance(Config *cfg, ChainContext *) {
    return new DMCRLS(cfg);
  }

  DataPoint *DMCRLS::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "rls")
      return NULL;
    return new DataPointRLS(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  {"rls", 0, &Arc::DMCRLS::Instance},
  {NULL, 0, NULL}
};
