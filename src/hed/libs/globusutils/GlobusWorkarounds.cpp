#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mutex>
#include <iostream>

#include <openssl/objects.h>
#include <openssl/x509v3.h>

#include <globus_gsi_callback.h>

#include <arc/Utils.h>

#include "GlobusErrorUtils.h"
#include "GlobusWorkarounds.h"

namespace Arc {

  static std::mutex lock_;

  bool GlobusRecoverProxyOpenSSL(void) {
    // No harm even if not needed - shall trun proxies on for code
    // which was written with no proxies in mind
    SetEnv("OPENSSL_ALLOW_PROXY_CERTS","1");
    return true;
  }

  bool GlobusPrepareGSSAPI(void) {
    std::unique_lock<std::mutex> lock(lock_);
    int index = -1;
    GlobusResult(globus_gsi_callback_get_X509_STORE_callback_data_index(&index));
    GlobusResult(globus_gsi_callback_get_SSL_callback_data_index(&index));
    return true;
  }

  bool GlobusModuleActivate(globus_module_descriptor_t* module) {
    std::unique_lock<std::mutex> lock(lock_);
    return (GlobusResult(globus_module_activate(module)));
  }
}

