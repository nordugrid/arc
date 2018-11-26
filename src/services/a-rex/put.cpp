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


static bool write_file(Arc::FileAccess& h,char* buf,size_t size) {
  for(;size>0;) {
    ssize_t l = h.fa_write(buf,size);
    if(l == -1) return false;
    size-=l; buf+=l;
  };
  return true;
}

Arc::MCC_Status ARexService::PutInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  return make_http_fault(outmsg,501,"Not Implemented");
}

Arc::MCC_Status ARexService::DeleteInfo(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  return make_http_fault(outmsg,501,"Not Implemented");
}

Arc::MCC_Status ARexService::PutCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  return make_http_fault(outmsg,501,"Not Implemented");
}

Arc::MCC_Status ARexService::DeleteCache(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& subpath) {
  return make_http_fault(outmsg,501,"Not Implemented");
}

static Arc::MCC_Status PutJobFile(Arc::Message& outmsg, Arc::FileAccess& file, std::string& errstr,
                                  Arc::PayloadStreamInterface& stream, FileChunks& fc, bool& complete) {
  complete = false;
  // TODO: Use memory mapped file to minimize number of in memory copies
  const int bufsize = 1024*1024;
  if(!fc.Size()) fc.Size(stream.Size());
  off_t pos = stream.Pos(); 
  if(file.fa_lseek(pos,SEEK_SET) != pos) {
    std::string err = Arc::StrError();
    errstr = "failed to set position of file to "+Arc::tostring(pos)+" - "+err;
    return ARexService::make_http_fault(outmsg, 500, "Error seeking to specified position in file");
  };
  char* buf = new char[bufsize];
  if(!buf) {
    errstr = "failed to allocate memory";
    return ARexService::make_http_fault(outmsg, 500, "Error allocating memory");
  };
  bool got_something = false;
  for(;;) {
    int size = bufsize;
    if(!stream.Get(buf,size)) break;
    if(size > 0) got_something = true;
    if(!write_file(file,buf,size)) {
      std::string err = Arc::StrError();
      delete[] buf;
      errstr = "failed to write to file - "+err;
      return ARexService::make_http_fault(outmsg, 500, "Error writing to file");
    };
    if(size) fc.Add(pos,size);
    pos+=size;
  };
  delete[] buf;
  // Due to limitation of PayloadStreamInterface it is not possible to
  // directly distingush between zero sized file and file with undefined
  // size. But by applying some dynamic heuristics it is possible.
  // TODO: extend/modify PayloadStreamInterface.
  if((stream.Size() == 0) && (stream.Pos() == 0) && (!got_something)) {
    complete = true;
  }
  return ARexService::make_empty_response(outmsg);
}

static Arc::MCC_Status PutJobFile(Arc::Message& outmsg, Arc::FileAccess& file, std::string& errstr,
                                  Arc::PayloadRawInterface& buf, FileChunks& fc, bool& complete) {
  complete = false;
  bool got_something = false;
  if(!fc.Size()) fc.Size(buf.Size());
  for(int n = 0;;++n) {
    char* sbuf = buf.Buffer(n);
    if(sbuf == NULL) break;
    off_t offset = buf.BufferPos(n);
    off_t size = buf.BufferSize(n);
    if(size > 0) {
      got_something = true;
      off_t o = file.fa_lseek(offset,SEEK_SET);
      if(o != offset) {
        std::string err = Arc::StrError();
        errstr = "failed to set position of file to "+Arc::tostring(offset)+" - "+err;
        return ARexService::make_http_fault(outmsg, 500, "Error seeking to specified position");
      };
      if(!write_file(file,sbuf,size)) {
        std::string err = Arc::StrError();
        errstr = "failed to write to file - "+err;
        return ARexService::make_http_fault(outmsg, 500, "Error writing file");
      };
      if(size) fc.Add(offset,size);
    };
  };
  if((buf.Size() == 0) && (!got_something)) {
    complete = true;
  }
  return ARexService::make_empty_response(outmsg);
}

Arc::MCC_Status ARexService::PutJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  // Nothing can be put into root endpoint
  if(id.empty()) return make_http_fault(outmsg, 500, "No job specified");
  // Check for proper payload
  Arc::MessagePayload* payload = inmsg.Payload();
  if(!payload) {
    logger_.msg(Arc::ERROR, "%s: put file %s: there is no payload", id, subpath);
    return make_http_fault(outmsg, 500, "Missing payload");
  };
  Arc::PayloadStreamInterface* stream = dynamic_cast<Arc::PayloadStreamInterface*>(payload);
  Arc::PayloadRawInterface* buf = dynamic_cast<Arc::PayloadRawInterface*>(payload);
  if((!stream) && (!buf)) {
    logger_.msg(Arc::ERROR, "%s: put file %s: unrecognized payload", id, subpath);
    return make_http_fault(outmsg, 500, "Error processing payload");
  }
  // Acquire job
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "%s: there is no such job: %s", job.ID(), job.Failure());
    return make_http_fault(outmsg, 500, "Job does not exist");
  };

  // Prepare access to file 
  FileChunksRef fc(files_chunks_.Get(job.ID()+"/"+subpath));
  Arc::FileAccess* file = job.CreateFile(subpath.c_str());
  if(file == NULL) {
    // TODO: report something
    logger_.msg(Arc::ERROR, "%s: put file %s: failed to create file: %s", job.ID(), subpath, job.Failure());
    return make_http_fault(outmsg, 500, "Error creating file");
  };

  Arc::MCC_Status r;
  std::string err;
  bool complete(false);
  if(stream) {
    r = PutJobFile(outmsg,*file,err,*stream,*fc,complete);
  } else {
    r = PutJobFile(outmsg,*file,err,*buf,*fc,complete);
  }
  file->fa_close(); Arc::FileAccess::Release(file);
  if(r) {
    if(complete || fc->Complete()) job.ReportFileComplete(subpath);
  } else {
    logger_.msg(Arc::ERROR, "%s: put file %s: %s", job.ID(), subpath, err);
  }
  return r;
} 

Arc::MCC_Status ARexService::DeleteJob(Arc::Message& inmsg,Arc::Message& outmsg,ARexGMConfig& config,std::string const& id,std::string const& subpath) {
  // Nothing can be removed in root endpoint
  if(id.empty()) return make_http_fault(outmsg, 500, "No job specified");
  // Ignoring payload
  // Acquire job
  ARexJob job(id,config,logger_);
  if(!job) {
    // There is no such job
    logger_.msg(Arc::ERROR, "%s: there is no such job: %s", job.ID(), job.Failure());
    return make_http_fault(outmsg, 500, "Job does not exist");
  };

  std::string full_path = job.GetFilePath(subpath.c_str());
  if(full_path.empty()) {
    logger_.msg(Arc::ERROR, "%s: delete file %s: failed to obtain file path: %s", job.ID(), subpath, job.Failure());
    return make_http_fault(outmsg, 500, "Error deleting file");
  };
  bool is_file = true;
  Arc::FileAccess* fs = job.OpenFile(subpath.c_str(), false, true);
  if(fs == NULL) {
    is_file = false;
    fs = job.OpenDir(subpath.c_str());
  }
  if(fs == NULL) {
    // TODO: report something
    logger_.msg(Arc::ERROR, "%s: delete file %s: failed to open file/dir: %s", job.ID(), subpath, job.Failure());
    return make_http_fault(outmsg, 500, "Error deleting file");
  };
  bool unlink_result = is_file ? fs->fa_unlink(full_path.c_str()) : fs->fa_rmdir(full_path.c_str());;
  int unlink_err = fs->geterrno();
  is_file ? fs->fa_close() : fs->fa_closedir(); Arc::FileAccess::Release(fs);

  if(!unlink_result) {
    if((unlink_err == ENOTDIR) || (unlink_err == ENOENT)) {
      return make_http_fault(outmsg, 404, "File not found");
    } else {
      return make_http_fault(outmsg, 500, "Error deleting file");
    };
  };

  return ARexService::make_empty_response(outmsg);
}

} // namespace ARex

