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
   // TODO: Finish this method
   //
   // Method for grid-manager cache checking 
   //
   //  You should call CheckCreated() with the URL of the file you want to check - eg CheckCreated("http://..."). File() is called within this method to resolve the URL to the local file name.
   //
   //  This information is from David Cameron
   //
   
   {
     std::string s;
     in.GetXML(s);
     logger.msg(Arc::DEBUG, "CachCheck: request = \n%s", s); 
   };  

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
     exit(1);
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
      Arc::XMLNode id = in["TheseFilesNeedToCheck"]["FileURL"][n];
      if(!id) break;
      
      fileexist = false;

	  std::string fileurl = ((std::string)in["TheseFilesNeedToCheck"]["FileURL"][n]);
         
      fileexist = (*cache).CheckCreated(fileurl);

      Arc::XMLNode file;

	  file.NewChild("FileURL") = fileurl;
	  file.NewChild("ExistInTheCache") = (fileexist ? "true": "false");


      Arc::XMLNode resultelement("Result");

      resultelement.NewChild(file);

      results.NewChild(resultelement);
   }
  

   // TESTING: file_owner_username

      fileexist = false;

	  std::string fileurl = "http://knowarc1.grid.niif.hu/storage/Makefile";
         
      fileexist = (*cache).CheckCreated(fileurl);

      logger.msg(Arc::INFO, "Cache check working or not: %s", fileexist ? "true": "false"); 

   {
     std::string s;
     out.GetXML(s);
     logger.msg(Arc::DEBUG, "CachCheck: response = \n%s", s); 
   };  

   return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 
