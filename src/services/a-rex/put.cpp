#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <fstream>

#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadRaw.h>
#include "PayloadFile.h"
#include "job.h"

#include "arex.h"

#define MAX_CHUNK_SIZE (10*1024*1024)

namespace ARex {

static Arc::MCC_Status http_put(ARexJob& job,const std::string& hpath,Arc::Logger& logger,Arc::PayloadStreamInterface& stream);
static Arc::MCC_Status http_put(ARexJob& job,const std::string& hpath,Arc::Logger& logger,Arc::PayloadRawInterface& buf);

// TODO: monitor chunks written into files and report when file is complete
Arc::MCC_Status ARexService::Put(Arc::Message& inmsg,Arc::Message& /*outmsg*/,ARexGMConfig& config,const std::string& id,const std::string& subpath) {
  if(id.empty()) return Arc::MCC_Status();
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "Put: there is no job: %s - %s", id, job.Failure());
    // TODO: make proper html message
    return Arc::MCC_Status();
  };
  Arc::MessagePayload* payload = inmsg.Payload();
  if(!payload) {
    logger_.msg(Arc::ERROR, "Put: there is no payload for file %s in job: %s", subpath, id);
    return Arc::MCC_Status();
  };
  Arc::PayloadStreamInterface* stream = NULL;
  try {
    stream = dynamic_cast<Arc::PayloadStreamInterface*>(payload);
  } catch(std::exception& e) { };
  if(stream) return http_put(job,subpath,logger_,*stream);
  Arc::PayloadRawInterface* buf = NULL;
  try {
    buf = dynamic_cast<Arc::PayloadRawInterface*>(payload);
  } catch(std::exception& e) { };
  if(buf) return http_put(job,subpath,logger_,*buf);
  logger_.msg(Arc::ERROR, "Put: unrecognized payload for file %s in job: %s", subpath, id);
  return Arc::MCC_Status();
} 

static bool write_file(int h,char* buf,size_t size) {
  for(;size>0;) {
    ssize_t l = write(h,buf,size);
    if(l == -1) return false;
    size-=l; buf+=l;
  };
  return true;
}

static Arc::MCC_Status http_put(ARexJob& job,const std::string& hpath,Arc::Logger& logger,Arc::PayloadStreamInterface& stream) {
  // TODO: Use memory mapped file to minimize number of in memory copies
  // File 
  const int bufsize = 1024*1024;
  int h = job.CreateFile(hpath.c_str());
  if(h == -1) {
    // TODO: report something
    logger.msg(Arc::ERROR, "Put: failed to create file %s for job %s - %s", hpath, job.ID(), job.Failure());
    return Arc::MCC_Status();
  };
  int pos = stream.Pos(); 
  if(lseek(h,pos,SEEK_SET) != pos) {
    std::string err = Arc::StrError();
    ::close(h);
    logger.msg(Arc::ERROR, "Put: failed to set position of file %s for job %s to %i - %s", hpath, job.ID(), pos, err);
    return Arc::MCC_Status();
  };
  char* buf = new char[bufsize];
  if(!buf) {
    ::close(h);
    logger.msg(Arc::ERROR, "Put: failed to allocate memory for file %s in job %s", hpath, job.ID());
    return Arc::MCC_Status();
  };
  for(;;) {
    int size = bufsize;
    if(!stream.Get(buf,size)) break;
    if(!write_file(h,buf,size)) {
      std::string err = Arc::StrError();
      delete[] buf; ::close(h);
      logger.msg(Arc::ERROR, "Put: failed to write to file %s for job %s - %s", hpath, job.ID(), err);
      return Arc::MCC_Status();
    };
  };
  delete[] buf; ::close(h);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static Arc::MCC_Status http_put(ARexJob& job,const std::string& hpath,Arc::Logger& logger,Arc::PayloadRawInterface& buf) {
  // File 
  int h = job.CreateFile(hpath.c_str());
  if(h == -1) {
    // TODO: report something
    logger.msg(Arc::ERROR, "Put: failed to create file %s for job %s - %s", hpath, job.ID(), job.Failure());
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
        ::close(h);
        return Arc::MCC_Status();
      };
      if(!write_file(h,sbuf,size)) {
        ::close(h);
        return Arc::MCC_Status();
      };
    };
  };
  ::close(h);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

