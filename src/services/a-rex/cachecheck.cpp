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

   Arc::XMLNode resp = out.NewChild("CacheCheckResponse");

   Arc::XMLNode results = resp.NewChild("CacheCheckResult");


   bool fileexist;

   Arc::FileCache * cache;

   cache = new Arc::FileCache();


/*

  std::string file_owner_username = "";

    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwuid_r(getuid(),&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
      olog<<"Wrong user name"<<std::endl; exit(1);
    }   
    if(pw->pw_name) file_owner_username=pw->pw_name;

   CacheConfig * cache_config = new CacheConfig(file_owner_username);
*/
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

   {
     std::string s;
     out.GetXML(s);
     logger.msg(Arc::DEBUG, "CachCheck: response = \n%s", s); 
   };  

   return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex 
