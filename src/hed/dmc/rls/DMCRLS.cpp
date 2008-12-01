#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

extern "C" {
#include "globus_rls_client.h"
}

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMCLoader.h>

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

  Plugin *DMCRLS::Instance(PluginArgument* arg) {
    Arc::DMCPluginArgument* dmcarg =
            arg?dynamic_cast<Arc::DMCPluginArgument*>(arg):NULL;
    if(!dmcarg) return NULL;
    return new DMCRLS((Arc::Config*)(*dmcarg));
  }

  DataPoint *DMCRLS::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "rls")
      return NULL;
    return new DataPointRLS(url);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  {"rls", "HED:DMC", 0, &Arc::DMCRLS::Instance},
  {NULL, NULL, 0, NULL}
};
