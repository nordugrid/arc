#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/loader/DMCLoader.h>

#include "DMCLDAP.h"
#include "DataPointLDAP.h"

namespace Arc {

  Logger DMCLDAP::logger(DMC::logger, "LDAP");

  DMCLDAP::DMCLDAP(Config *cfg) : DMC(cfg) {
    Register(this);
  }

  DMCLDAP::~DMCLDAP() {
    Unregister(this);
  }

  DMC* DMCLDAP::Instance(Arc::Config *cfg, Arc::ChainContext*) {
    return new DMCLDAP(cfg);
  }

  DataPoint* DMCLDAP::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "ldap") return NULL;
    return new DataPointLDAP(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  { "ldap", 0, &Arc::DMCLDAP::Instance },
  { NULL, 0, NULL }
};
