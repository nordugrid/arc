#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <string>

#include <arc/StringConv.h>
#include <arc/message/PayloadRaw.h>
#include "PayloadFile.h"
#include "job.h"

#include "arex.h"

#define MAX_CHUNK_SIZE (10*1024*1024)

namespace ARex {

static Arc::MCC_Status http_get(Arc::Message& outmsg,const std::string& burl,const std::string& bpath,std::string hpath);

Arc::MCC_Status ARexService::Get(Arc::Message& outmsg,ARexGMConfig& config,const std::string& id,const std::string& subpath) {
  if(id.empty()) {
    // Make list of jobs
    std::string html;
    html="<HTML>\r\n<HEAD>ARex: Jobs list</HEAD>\r\n<BODY><UL>\r\n";
    std::list<std::string> jobs = ARexJob::Jobs(config);
    //std::cerr<<"Get: endpoint: "<<config.Endpoint()<<std::endl;
    for(std::list<std::string>::iterator job = jobs.begin();job!=jobs.end();++job) {
      std::string line = "<LI><I>job</I> <A HREF=\"";
      //std::cerr<<"Get: job: "<<(*job)<<std::endl;
      line+=config.Endpoint()+"/"+(*job);
      line+="\">";
      line+=(*job);
      line+="</A>\r\n";
      html+=line;
    };
    html+="</UL>\r\n</BODY>\r\n</HTML>";
    //std::cerr<<"Get: html: "<<html<<std::endl;
    Arc::PayloadRaw* buf = NULL;
    buf=new Arc::PayloadRaw;
    if(buf) buf->Insert(html.c_str(),0,html.length());
    outmsg.Payload(buf);
    outmsg.Attributes()->set("HTTP:content-type","text/html");
    return Arc::MCC_Status(Arc::STATUS_OK);
  };
  ARexJob job(id,config);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Get: there is no job %s - %s", id.c_str(), job.Failure().c_str());
    // TODO: make proper html message
    return Arc::MCC_Status();
  };
  std::string session_dir = job.SessionDir();
  Arc::PayloadRawInterface* buf = NULL;
  if(!http_get(outmsg,config.Endpoint()+"/"+id,session_dir,subpath)) {
    // Can't get file
    logger.msg(Arc::ERROR, "Get: can't process file %s/%s", session_dir.c_str(), subpath.c_str());
    // TODO: make proper html message
    return Arc::MCC_Status();
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
} 

static Arc::MCC_Status http_get(Arc::Message& outmsg,const std::string& burl,const std::string& bpath,std::string hpath) {
  std::string path=bpath;
  if(!hpath.empty()) if(hpath[0] == '/') hpath=hpath.substr(1);
  if(!hpath.empty()) if(hpath[hpath.length()-1] == '/') hpath.resize(hpath.length()-1);
  if(!hpath.empty()) path+="/"+hpath;
  //std::cerr<<"http:get: burl: "<<burl<<std::endl;
  //std::cerr<<"http:get: bpath: "<<bpath<<std::endl;
  //std::cerr<<"http:get: hpath: "<<hpath<<std::endl;
  //std::cerr<<"http:get: path: "<<path<<std::endl;
  struct stat st;
  if(lstat(path.c_str(),&st) == 0) {
    if(S_ISDIR(st.st_mode)) {
      //std::cerr<<"http:get: directory"<<std::endl;
      DIR *dir=opendir(path.c_str());
      if(dir != NULL) {
        // Directory - html with file list
        struct dirent file_;
        struct dirent *file;
        std::string html;
        html="<HTML>\r\n<HEAD>ARex: Job</HEAD>\r\n<BODY><UL>\r\n";
	std::string furl = burl;
	if(!hpath.empty()) furl+="/"+hpath;
        for(;;) {
          readdir_r(dir,&file_,&file);
          if(file == NULL) break;
          if(strcmp(file->d_name,".") == 0) continue;
          if(strcmp(file->d_name,"..") == 0) continue;
          std::string fpath = path+"/"+file->d_name;
          if(lstat(fpath.c_str(),&st) == 0) {
            if(S_ISREG(st.st_mode)) {
              std::string line = "<LI><I>file</I> <A HREF=\"";
              line+=furl+"/"+file->d_name;
              line+="\">";
              line+=file->d_name;
              line+="</A> - "+Arc::tostring(st.st_size)+" bytes"+"\r\n";
              html+=line;
            } else if(S_ISDIR(st.st_mode)) {
              std::string line = "<LI><I>dir</I> <A HREF=\"";
              line+=furl+"/"+file->d_name+"/";
              line+="\">";
              line+=file->d_name;
              line+="</A>\r\n";
              html+=line;
            };
          };
        };
        closedir(dir);
        html+="</UL>\r\n</BODY>\r\n</HTML>";
        //std::cerr<<"Get: html: "<<html<<std::endl;
        Arc::PayloadRaw* buf = new Arc::PayloadRaw;
        if(buf) buf->Insert(html.c_str(),0,html.length());
        outmsg.Payload(buf);
        outmsg.Attributes()->set("HTTP:content-type","text/html");
        return Arc::MCC_Status(Arc::STATUS_OK);
      };
    } else if(S_ISREG(st.st_mode)) {
      // File 
      //std::cerr<<"http:get: file: "<<path<<std::endl;
      //size=st.st_size;
      // TODO: support for range requests
      PayloadFile* h = new PayloadFile(path.c_str());
      outmsg.Payload(h);
      outmsg.Attributes()->set("HTTP:content-type","application/octet-stream");
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  // Can't process this path
  // offset=0; size=0;
  return Arc::MCC_Status();
}

} // namespace ARex

