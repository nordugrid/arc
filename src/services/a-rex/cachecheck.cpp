#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h> 

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/URL.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>

#include "job.h"
#include "arex.h"

#define CACHE_CHECK_SESSION_DIR_ID "9999999999999999999999999999999"

namespace ARex {

Arc::MCC_Status ARexService::CacheCheck(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {

  std::vector<std::string> caches;
  std::vector<std::string> draining_caches;
  std::vector<std::string> readonly_caches;
  // use cache dir(s) from conf file
  try {
    CacheConfig cache_config(config.GmConfig().CacheParams());
    cache_config.substitute(config.GmConfig(), config.User());
    caches = cache_config.getCacheDirs();
    readonly_caches = cache_config.getReadOnlyCacheDirs();
  }
  catch (CacheConfigException& e) {
    logger.msg(Arc::ERROR, "Error with cache configuration: %s", e.what());
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Error with cache configuration");
    fault.Detail(true).NewChild("CacheConfigurationFault");
    out.Destroy();
    return Arc::MCC_Status();
  }

  if (caches.empty()) {
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Cache is disabled");
    fault.Detail(true).NewChild("CacheDisabledFault");
    out.Destroy();
    return Arc::MCC_Status();
  }

  Arc::FileCache cache(caches, draining_caches, readonly_caches, CACHE_CHECK_SESSION_DIR_ID ,config.User().get_uid(), config.User().get_gid());
  if (!cache) {
    logger.msg(Arc::ERROR, "Error with cache configuration");
    Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Error with cache configuration");
    fault.Detail(true).NewChild("CacheConfigurationFault");
    out.Destroy();
    return Arc::MCC_Status();
  }

  bool fileexist;
  Arc::XMLNode resp = out.NewChild("CacheCheckResponse");
  Arc::XMLNode results = resp.NewChild("CacheCheckResult");

  for(int n = 0;;++n) {
    Arc::XMLNode id = in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];

    if (!id) break;

    std::string fileurl = (std::string)in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];
    Arc::XMLNode resultelement = results.NewChild("Result");
    fileexist = false;
    std::string file_lfn;
    Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::SkipCredentials);
    Arc::UserConfig usercfg(cred_type);
    Arc::URL url(fileurl);
    Arc::DataHandle d(url, usercfg);

    logger.msg(Arc::INFO, "Looking up URL %s", d->str());
    file_lfn = cache.File(d->str());
    logger.msg(Arc::INFO, "Cache file is %s", file_lfn);

    struct stat fileStat;

    fileexist = (stat(file_lfn.c_str(), &fileStat) == 0) ? true : false;
    resultelement.NewChild("FileURL") = fileurl;
    resultelement.NewChild("ExistInTheCache") = (fileexist ? "true": "false");
    if (fileexist)
      resultelement.NewChild("FileSize") = Arc::tostring(fileStat.st_size);
    else
      resultelement.NewChild("FileSize") = "0";
  }
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 
