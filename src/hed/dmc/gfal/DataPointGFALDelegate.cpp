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

#include "DataPointGFALDelegate.h"

namespace ArcDMCGFAL {

  using namespace Arc;


  DataPointGFALDelegate::DataPointGFALDelegate(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDelegate("dmcgfal", url, usercfg, parg) {
  }

  DataPointGFALDelegate::~DataPointGFALDelegate() {
  }

  Plugin* DataPointGFALDelegate::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg) return NULL;
    if (((const URL &)(*dmcarg)).Protocol() != "rfio" &&
        ((const URL &)(*dmcarg)).Protocol() != "dcap" &&
        ((const URL &)(*dmcarg)).Protocol() != "gsidcap" &&
        ((const URL &)(*dmcarg)).Protocol() != "lfc" &&
        // gfal protocol is used in 3rd party transfer to load this DMC
        ((const URL &)(*dmcarg)).Protocol() != "gfal") {
      return NULL;
    }
    return new DataPointGFALDelegate(*dmcarg, *dmcarg, dmcarg);
  }

  bool DataPointGFALDelegate::RequiresCredentials() const {
    return true;
  }

} // namespace ArcDMCGFAL

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "gfal2", "HED:DMC", "Grid File Access Library 2", 0, &ArcDMCGFAL::DataPointGFALDelegate::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
  }
}

