#include "DMCGridFTP.h"
#include "DataPointGridFTP.h"
#include "common/Logger.h"
#include "common/URL.h"
#include "loader/DMCLoader.h"

#include "globus_ftp_client.h"

namespace Arc {

  Logger DMCGridFTP::logger(DMC::logger, "GridFTP");

  DMCGridFTP::DMCGridFTP(Config *cfg) : DMC(cfg) {
    globus_module_activate(GLOBUS_FTP_CLIENT_MODULE);
    Register(this);
  }

  DMCGridFTP::~DMCGridFTP() {
    Unregister(this);
    globus_module_deactivate(GLOBUS_FTP_CLIENT_MODULE);
  }

  DMC* DMCGridFTP::Instance(Arc::Config *cfg, Arc::ChainContext *ctx) {
    return new DMCGridFTP(cfg);
  }

  DataPoint* DMCGridFTP::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "gsiftp" &&
	url.Protocol() != "ftp") return NULL;
    return new DataPointGridFTP(url);
  }

} // namespace Arc

dmc_descriptors ARC_DMC_LOADER = {
  { "gridftp", 0, &Arc::DMCGridFTP::Instance },
  { NULL, 0, NULL }
};
