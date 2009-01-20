#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/data/FileCache.h>
#include <sys/types.h>
#include <pwd.h>

#include "arex.h"
#include "grid-manager/conf/conf_cache.h"
#include "grid-manager/jobs/job.h"
#include "grid-manager/jobs/users.h"

#define CACHE_CHECK_SESSION_DIR_ID "9999999999999999999999999999999"

namespace ARex {

Arc::MCC_Status ARexService::CacheCheck(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {

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
     logger.msg(Arc::ERROR, "Wrong user name"); 
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
     logger.msg(Arc::ERROR, "Will not use caching"); 
    }

  Arc::FileCache * cache;
  if(!caches.empty()) {

    cache = new Arc::FileCache(caches, CACHE_CHECK_SESSION_DIR_ID ,uid,gid);
    if (!(*cache)) {
     logger.msg(Arc::ERROR, "Error creating cache"); 
      exit(1);
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

      fileexist = (*cache).CheckCreated(fileurl);

      Arc::XMLNode resultelement = results.NewChild("Result");

	  resultelement.NewChild("FileURL") = fileurl;
	  resultelement.NewChild("ExistInTheCache") = (fileexist ? "true": "false");

   }
  
   return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 
