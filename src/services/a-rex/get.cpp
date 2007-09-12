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

static Arc::PayloadRawInterface* http_get(const std::string& burl,const std::string& bpath,const std::string& hpath);

Arc::PayloadRawInterface* ARexService::Get(ARexGMConfig& config,const std::string& id,const std::string& subpath) {
  if(id.empty()) {
    // Make list of jobs
    std::string html;
    html="<HTML>\r\n<HEAD>ARex: Jobs list</HEAD>\r\n<BODY><UL>\r\n";
    std::list<std::string> jobs = ARexJob::Jobs(config);
std::cerr<<"Get: endpoint: "<<config.Endpoint()<<std::endl;
    for(std::list<std::string>::iterator job = jobs.begin();job!=jobs.end();++job) {
      std::string line = "<LI><I>job</I> <A HREF=\"";
std::cerr<<"Get: job: "<<(*job)<<std::endl;
      line+=config.Endpoint()+"/"+(*job);
      line+="\">";
      line+=(*job);
      line+="</A>\r\n";
      html+=line;
    };
    html+="</UL>\r\n</BODY>\r\n</HTML>";
std::cerr<<"Get: html: "<<html<<std::endl;
    Arc::PayloadRaw* buf = NULL;
    buf=new Arc::PayloadRaw;
    if(buf) buf->Insert(html.c_str(),0,html.length());
    return buf;
  };
  ARexJob job(id,config);
  if(!job) {
    // There is no such job
std::cerr<<"Get: there is no job: "<<id<<std::endl;
    // TODO: make proper html message
    return NULL;
  };
  std::string session_dir = job.SessionDir();
  Arc::PayloadRawInterface* buf = NULL;
  if((buf=http_get(config.Endpoint()+"/"+id,session_dir,subpath)) == NULL) {
    // Can't get file
    // TODO: make proper html message
    return NULL;
  };
  return buf;
} 

static Arc::PayloadRawInterface* http_get(const std::string& burl,const std::string& bpath,const std::string& hpath) {
  std::string path=bpath+"/"+hpath;
std::cerr<<"http:get: burl: "<<burl<<std::endl;
std::cerr<<"http:get: bpath: "<<bpath<<std::endl;
std::cerr<<"http:get: hpath: "<<hpath<<std::endl;
std::cerr<<"http:get: path: "<<path<<std::endl;
  struct stat64 st;
  if(lstat64(path.c_str(),&st) == 0) {
    if(S_ISDIR(st.st_mode)) {
std::cerr<<"http:get: directory"<<std::endl;
      DIR *dir=opendir(path.c_str());
      if(dir != NULL) {
        // Directory - html with file list
        struct dirent file_;
        struct dirent *file;
        std::string html;
        html="<HTML>\r\n<HEAD>ARex: Job</HEAD>\r\n<BODY><UL>\r\n";
        for(;;) {
          readdir_r(dir,&file_,&file);
          if(file == NULL) break;
          if(strcmp(file->d_name,".") == 0) continue;
          if(strcmp(file->d_name,"..") == 0) continue;
          std::string fpath = path+"/"+file->d_name;
          if(lstat64(fpath.c_str(),&st) == 0) {
            if(S_ISREG(st.st_mode)) {
              std::string line = "<LI><I>file</I> <A HREF=\"";
              line+=burl+"/"+hpath+"/"+file->d_name;
              line+="\">";
              line+=file->d_name;
              line+="</A> - "+Arc::tostring(st.st_size)+" bytes"+"\r\n";
              html+=line;
            } else if(S_ISDIR(st.st_mode)) {
              std::string line = "<LI><I>dir</I> <A HREF=\"";
              line+=burl+"/"+hpath+"/"+file->d_name+"/";
              line+="\">";
              line+=file->d_name;
              line+="</A>\r\n";
              html+=line;
            };
          };
        };
        closedir(dir);
        html+="</UL>\r\n</BODY>\r\n</HTML>";
std::cerr<<"Get: html: "<<html<<std::endl;
        Arc::PayloadRaw* buf = new Arc::PayloadRaw;
        if(buf) buf->Insert(html.c_str(),0,html.length());
        return buf;
      };
    } else if(S_ISREG(st.st_mode)) {
      // File 
std::cerr<<"http:get: file: "<<path<<std::endl;
      //size=st.st_size;
      // TODO: support for range requests
      PayloadFile* h = new PayloadFile(path.c_str());
      return h;
/*
      int h = open64(path.c_str(),O_RDONLY);
      if(h != -1) {
std::cerr<<"http:get: file is opened"<<std::endl;
        off_t o = lseek64(h,offset,SEEK_SET);
        if(o != offset) {
          // Out of file
std::cerr<<"http:get: file has ended"<<std::endl;

        } else {
          if(size > MAX_CHUNK_SIZE) size=MAX_CHUNK_SIZE;
std::cerr<<"http:get: file: size: "<<size<<std::endl;
          char* sbuf = buf.Insert(offset,size);
          if(sbuf) {
            ssize_t l = read(h,sbuf,size);
            if(l != -1) {
              close(h);
              size=l;
              buf.Truncate(offset+size); // Make sure there is no garbage in buffer
              buf.Truncate(st.st_size);  // Specify logical size
              return true;
            };
          };
        };
        close(h);
      };
*/
    };
  };
  // Can't process this path
  // offset=0; size=0;
  return NULL;
}

}; // namespace ARex

