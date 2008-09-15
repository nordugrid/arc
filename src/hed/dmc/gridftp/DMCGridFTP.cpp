#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "globus_ftp_client.h"

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/loader/DMCLoader.h>

#include "DataPointGridFTP.h"
#include "DMCGridFTP.h"

namespace Arc {

  Logger DMCGridFTP::logger(DMC::logger, "GridFTP");

  DMCGridFTP::DMCGridFTP(Config *cfg)
    : DMC(cfg) {
    globus_module_activate(GLOBUS_FTP_CLIENT_MODULE);
    Register(this);
  }

  DMCGridFTP::~DMCGridFTP() {
    Unregister(this);
    globus_module_deactivate(GLOBUS_FTP_CLIENT_MODULE);
  }

  DMC* DMCGridFTP::Instance(Config *cfg, ChainContext*) {
    return new DMCGridFTP(cfg);
  }

  DataPoint* DMCGridFTP::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "gsiftp" && url.Protocol() != "ftp")
      return NULL;
    return new DataPointGridFTP(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  { "gridftp", 0, &Arc::DMCGridFTP::Instance },
  { NULL, 0, NULL }
};
