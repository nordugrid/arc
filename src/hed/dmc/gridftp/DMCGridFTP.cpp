#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <globus_ftp_client.h>
#include <openssl/ssl.h>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/DMCLoader.h>
#include <arc/globusutils/GlobusWorkarounds.h>

#include "DataPointGridFTP.h"
#include "DMCGridFTP.h"

namespace Arc {

  static Glib::Mutex openssl_lock;
  static bool openssl_initialized = false;
  static bool proxy_initialized = false;

  Logger DMCGridFTP::logger(DMC::logger, "GridFTP");

  DMCGridFTP::DMCGridFTP(Config *cfg)
    : DMC(cfg) {
    globus_module_activate(GLOBUS_FTP_CLIENT_MODULE);
    if(!proxy_initialized) 
      proxy_initialized = GlobusRecoverProxyOpenSSL();
    Register(this);
  }

  DMCGridFTP::~DMCGridFTP() {
    Unregister(this);
    globus_module_deactivate(GLOBUS_FTP_CLIENT_MODULE);
  }

  Plugin* DMCGridFTP::Instance(PluginArgument* arg) {
    Arc::DMCPluginArgument* dmcarg =
            arg?dynamic_cast<Arc::DMCPluginArgument*>(arg):NULL;
    if(!dmcarg) return NULL;
    openssl_lock.lock();
    if(!openssl_initialized) {
      SSL_load_error_strings();
      if(!SSL_library_init()){
        logger.msg(ERROR, "SSL_library_init failed");
      } else {
        openssl_initialized = true;
      };
    };
    openssl_lock.unlock();
    return new DMCGridFTP((Arc::Config*)(*dmcarg));
  }

  DataPoint* DMCGridFTP::iGetDataPoint(const URL& url) {
    if (url.Protocol() != "gsiftp" && url.Protocol() != "ftp")
      return NULL;
    return new DataPointGridFTP(url);
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "gridftp", "HED:DMC", 0, &Arc::DMCGridFTP::Instance },
  { NULL, NULL, 0, NULL }
};
