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

Arc::MCC_Status ARexService::Put(Arc::Message& /*inmsg*/,Arc::Message& /*outmsg*/,ARexGMConfig& config,const std::string& id,const std::string& subpath,Arc::PayloadRawInterface& buf) {
  if(id.empty()) return Arc::MCC_Status();
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Put: there is no job: %s - %s", id, job.Failure());
    // TODO: make proper html message
    return Arc::MCC_Status();
  };
  return http_put(job,config.Endpoint()+"/"+id,subpath,buf);
} 

static Arc::MCC_Status http_put(ARexJob& job,const std::string& /*burl*/,const std::string& hpath,Arc::PayloadRawInterface& buf) {
  // File 
  int h = job.CreateFile(hpath.c_str());
  if(h == -1) {
    
    return Arc::MCC_Status();
  };
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
        if(l == -1) {
          close(h);
          return Arc::MCC_Status();
        };
        size-=l; sbuf+=l;
      };
    };
  };
  close(h);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

