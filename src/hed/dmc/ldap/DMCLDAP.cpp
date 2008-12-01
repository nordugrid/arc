#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMCLoader.h>

#include "DataPointLDAP.h"
#include "DMCLDAP.h"

namespace Arc {

  Logger DMCLDAP::logger(DMC::logger, "LDAP");

  DMCLDAP::DMCLDAP(Config *cfg)
    : DMC(cfg) {
    Register(this);
  }

  DMCLDAP::~DMCLDAP() {
    Unregister(this);
  }

  Plugin *DMCLDAP::Instance(PluginArgument* arg) {
    Arc::DMCPluginArgument* dmcarg =
            arg?dynamic_cast<Arc::DMCPluginArgument*>(arg):NULL;
    if(!dmcarg) return NULL;
    return new DMCLDAP((Arc::Config*)(*dmcarg));
  }

  DataPoint *DMCLDAP::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "ldap")
      return NULL;
    return new DataPointLDAP(url);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  {"ldap", "HED:DMC", 0, &Arc::DMCLDAP::Instance},
  {NULL, NULL, 0, NULL}
};
