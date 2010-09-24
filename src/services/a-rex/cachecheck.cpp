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

#include "arex.h"
#include "grid-manager/conf/conf_cache.h"
#include "grid-manager/jobs/job.h"
#include "grid-manager/jobs/users.h"

#define CACHE_CHECK_SESSION_DIR_ID "9999999999999999999999999999999"

namespace ARex {

Arc::MCC_Status ARexService::CacheCheck(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {

  // We are supporting only this cachedir format for checking: cachedir="/tmp/cache"
  // The cachedir="/tmp/%U/cache" format cannot be implemented at the moment 
  // but maybe at the future 

  uid_t uid = getuid();
  gid_t gid = getgid();
  std::string file_owner_username = "";
  JobUser user(*gm_env_,uid);
  std::vector<std::string> caches;
  struct passwd pw_;
  struct passwd *pw;
  char buf[BUFSIZ];
  getpwuid_r(getuid(),&pw_,buf,BUFSIZ,&pw);

  if(pw == NULL) {
     logger.msg(Arc::ERROR, "Error with cache configuration"); 
     Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Error with cache configuration");  
     fault.Detail(true).NewChild("CacheConfigurationFault");
	 out.Destroy();
	 return Arc::MCC_Status();
  }

  if(pw->pw_name) file_owner_username=pw->pw_name;
  // use cache dir(s) from conf file
  CacheConfig cache_config;
  try {
      cache_config = CacheConfig(*gm_env_,std::string(file_owner_username));
      std::vector<std::string> conf_caches = cache_config.getCacheDirs();
      // add each cache to our list
      for (std::vector<std::string>::iterator i = conf_caches.begin(); i != conf_caches.end(); i++) {
        user.substitute(*i);
        caches.push_back(*i);
     }
  }
  catch (CacheConfigException e) {
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

  Arc::FileCache * cache;
  if(!caches.empty()) {
    cache = new Arc::FileCache(caches, CACHE_CHECK_SESSION_DIR_ID ,uid,gid);
    if (!(*cache)) {
     logger.msg(Arc::ERROR, "Error with cache configuration"); 
     Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Error with cache configuration");  
     fault.Detail(true).NewChild("CacheConfigurationFault");
	 out.Destroy();
     delete cache;
	 return Arc::MCC_Status();
    }
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
      Arc::UserConfig usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials));
      Arc::URL url(fileurl);
      Arc::DataHandle d(url, usercfg);

      logger.msg(Arc::INFO, "Looking up URL %s", d->str());
      file_lfn = (*cache).File(d->str());
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
  if (cache) delete cache;
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 
