// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/ssl.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/data/DataBuffer.h>
#include <arc/CheckSum.h>
#include <arc/Run.h>
#include <arc/ArcLocation.h>

#include "DataPointGridFTPDelegate.h"

namespace ArcDMCGridFTP {

  using namespace Arc;


  DataPointGridFTPDelegate::DataPointGridFTPDelegate(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDelegate((Arc::ArcLocation::GetLibDir()+G_DIR_SEPARATOR_S+"arc-dmcgridftp").c_str(), std::list<std::string>(), url, usercfg, parg) {
    is_secure = false;
    if (url.Protocol() == "gsiftp") is_secure = true;
  }

  DataPointGridFTPDelegate::~DataPointGridFTPDelegate() {
  }

  Plugin* DataPointGridFTPDelegate::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg) return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "gsiftp" &&
        ((const URL&)(*dmcarg)).Protocol() != "ftp") {
      return NULL;
    }
    return new DataPointGridFTPDelegate(*dmcarg, *dmcarg, dmcarg);
  }

  bool DataPointGridFTPDelegate::RequiresCredentials() const {
    return is_secure;
  }

} // namespace ArcDMCGridFTP

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "gsiftp", "HED:DMC", "FTP or FTP with GSI security", 0, &ArcDMCGridFTP::DataPointGridFTPDelegate::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
  }
}

