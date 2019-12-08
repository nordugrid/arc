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

#include "DataPointXrootdDelegate.h"

namespace ArcDMCXrootd {

  using namespace Arc;


  DataPointXrootdDelegate::DataPointXrootdDelegate(const URL& url, const UserConfig& usercfg, PluginArgument* parg)
    : DataPointDelegate("dmcxrootd", url, usercfg, parg) {
  }

  DataPointXrootdDelegate::~DataPointXrootdDelegate() {
  }

  Plugin* DataPointXrootdDelegate::Instance(PluginArgument *arg) {
    DataPointPluginArgument *dmcarg = dynamic_cast<DataPointPluginArgument*>(arg);
    if (!dmcarg) return NULL;
    if (((const URL&)(*dmcarg)).Protocol() != "root") {
      return NULL;
    }
    return new DataPointXrootdDelegate(*dmcarg, *dmcarg, dmcarg);
  }

  bool DataPointXrootdDelegate::RequiresCredentials() const {
    return true;
  }

  bool DataPointXrootdDelegate::RequiresCredentialsInFile() const {
    return true;
  }

  bool DataPointXrootdDelegate::SupportsTransfer() const {
    return true;
  }

  DataStatus DataPointXrootdDelegate::List(std::list<FileInfo>& files, DataPoint::DataPointInfoType verb) {
    // Xrootd is slow when quering with higher verbosity. Here we simulate internal behavior of the 
    // plugin by performing Stat externally. Proper solution would be to push obtaineed infomation
    // about eah file as soon as it is obtained. But so far quick hack shuld be enough.
    if (verb <= INFO_TYPE_NAME) 
      return DataPointDelegate::List(files, verb);
    DataStatus result = DataPointDelegate::List(files, INFO_TYPE_NAME);
    if (!result) return result;
    URL url_orig = url;
    for(std::list<FileInfo>::iterator f = files.begin(); f != files.end(); ++f) {
      std::string name = f->GetName();
      url = Arc::URL(url_orig.plainstr() + '/' + name);
      Stat(*f, verb);
      f->SetName(name);
    }
    return result;
  }

} // namespace ArcDMCGridFTP

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "root", "HED:DMC", "XRootd", 0, &ArcDMCXrootd::DataPointXrootdDelegate::Instance },
  { NULL, NULL, NULL, 0, NULL }
};

extern "C" {
  void ARC_MODULE_CONSTRUCTOR_NAME(Glib::Module* module, Arc::ModuleManager* manager) {
  }
}

