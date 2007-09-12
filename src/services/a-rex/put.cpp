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

static Arc::MCC_Status http_put(ARexJob& job,const std::string& burl,const std::string& hpath,Arc::PayloadRawInterface& buf);

Arc::MCC_Status ARexService::Put(ARexGMConfig& config,const std::string& id,const std::string& subpath,Arc::PayloadRawInterface& buf) {
  if(id.empty()) return Arc::MCC_Status();
  ARexJob job(id,config);
  if(!job) {
    // There is no such job
std::cerr<<"Put: there is no job: "<<id<<std::endl;
    // TODO: make proper html message
    return Arc::MCC_Status();
  };
  uint64_t offset = 0;
  uint64_t size = MAX_CHUNK_SIZE;
  return http_put(job,config.Endpoint()+"/"+id,subpath,buf);
} 

static Arc::MCC_Status http_put(ARexJob& job,const std::string& burl,const std::string& hpath,Arc::PayloadRawInterface& buf) {
std::cerr<<"http:put: burl: "<<burl<<std::endl;
std::cerr<<"http:put: hpath: "<<hpath<<std::endl;
  // File 
  int h = job.CreateFile(hpath.c_str());
  if(h == -1) return Arc::MCC_Status();
std::cerr<<"http:put: file is opened"<<std::endl;
  for(int n = 0;;++n) {
    char* sbuf = buf.Buffer(n);
    if(sbuf == NULL) break;
    off_t offset = buf.BufferPos(n);
    size_t size = buf.BufferSize(n);
    if(size > 0) {
      off_t o = lseek(h,offset,SEEK_SET);
      if(o != offset) {
       close(h);
        return Arc::MCC_Status();
      };
      for(;size>0;) {
        ssize_t l = write(h,sbuf,size);
std::cerr<<"http:put: wrote: "<<l<<std::endl;
        if(l == -1) {
          close(h);
          return Arc::MCC_Status();
        };
        size-=l; sbuf+=l;
      };
    };
  };
  close(h);
std::cerr<<"http:put: success"<<std::endl;
  return Arc::MCC_Status(Arc::STATUS_OK);
}

}; // namespace ARex

