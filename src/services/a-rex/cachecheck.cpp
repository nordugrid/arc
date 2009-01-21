#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/data/FileCache.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h> 
#include <arc/data/DataPoint.h>
#include <arc/URL.h>
#include <arc/data/DMC.h>

#include "arex.h"
#include "grid-manager/conf/conf_cache.h"
#include "grid-manager/jobs/job.h"
#include "grid-manager/jobs/users.h"

#define CACHE_CHECK_SESSION_DIR_ID "9999999999999999999999999999999"

namespace ARex {

Arc::MCC_Status ARexService::CacheCheck(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {

      // We are supporting only this cachedir format for checking: cachedir="/tmp/cache"
	  //
	  // The cachedir="/tmp/%U/cache" format cannot be implemented at the moment 
	  // but maybe at the future 


  uid_t uid = getuid();
  gid_t gid = getgid();
 
  std::string file_owner_username = "";

  JobUser user(uid);

  std::vector<struct Arc::CacheParameters> caches;

    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwuid_r(getuid(),&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
     logger.msg(Arc::ERROR, "Error with cache configuration"); 
     Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Error with cache configuration");  
     fault.Detail(true).NewChild("CacheConfigurationFault");
	 out.Destroy();
	 return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
    if(pw->pw_name) file_owner_username=pw->pw_name;

    // use cache dir(s) from conf file
    try {
      CacheConfig * cache_config = new CacheConfig(file_owner_username);
      std::list<std::list<std::string> > conf_caches = cache_config->getCacheDirs();
      // add each cache to our list
      for (std::list<std::list<std::string> >::iterator i = conf_caches.begin(); i != conf_caches.end(); i++) {
        std::list<std::string>::iterator j = i->begin();
        struct Arc::CacheParameters cache_params;
        user.substitute(*j); cache_params.cache_path = (*j); j++;
        user.substitute(*j); cache_params.cache_job_dir_path = (*j); j++;
        if (j != i->end()) {user.substitute(*j); cache_params.cache_link_path = (*j);}
        else cache_params.cache_link_path = "";
        caches.push_back(cache_params);
      }
    }
    catch (CacheConfigException e) {
     logger.msg(Arc::ERROR, "Error with cache configuration: %s", e.what()); 
     Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Error with cache configuration");  
     fault.Detail(true).NewChild("CacheConfigurationFault");
	 out.Destroy();
	 return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }

  if (caches.empty()) {
     Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Cache is disabled");  
     fault.Detail(true).NewChild("CacheDisabledFault");
	 out.Destroy();
	 return Arc::MCC_Status(Arc::GENERIC_ERROR);
  }


  Arc::FileCache * cache;
  if(!caches.empty()) {

    cache = new Arc::FileCache(caches, CACHE_CHECK_SESSION_DIR_ID ,uid,gid);
    if (!(*cache)) {
     logger.msg(Arc::ERROR, "Error with cache configuration"); 
     Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Error with cache configuration");  
     fault.Detail(true).NewChild("CacheConfigurationFault");
	 out.Destroy();
	 return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
  }

  bool fileexist;

  Arc::XMLNode resp = out.NewChild("CacheCheckResponse");

  Arc::XMLNode results = resp.NewChild("CacheCheckResult");

   for(int n = 0;;++n) {
      Arc::XMLNode id = in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];
      
      if (!id) break;
    
      fileexist = false;

	  std::string fileurl = (std::string)in["CacheCheck"]["TheseFilesNeedToCheck"]["FileURL"][n];
 
      std::string file_lfn;

      Arc::DataPoint* d = Arc::DMC::GetDataPoint(fileurl);

      file_lfn = (*cache).File(d->str());

	  struct stat fileStat;
	  fileexist = (stat(file_lfn.c_str(), &fileStat) == 0) ? true : false;

      Arc::XMLNode resultelement = results.NewChild("Result");

	  resultelement.NewChild("FileURL") = fileurl;
	  resultelement.NewChild("ExistInTheCache") = (fileexist ? "true": "false");
   }
  
   return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 
