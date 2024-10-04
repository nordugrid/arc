#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/poll.h>

#include <arc/Utils.h>
#include "PayloadStream.h"

namespace Arc {

PayloadStream::PayloadStream(int h):timeout_(60),handle_(h),seekable_(false) {
  struct stat st;
  if(fstat(handle_,&st) != 0) return;
  if(!(S_ISREG(st.st_mode))) return;
  seekable_=true;
  return;
}

#define FAILSYSERR(msg) { \
  failure_ = MCC_Status(GENERIC_ERROR,"STREAM",(msg)+StrError(errno)); \
  return false; \
}

bool PayloadStream::Get(char* buf,int& size) {
  if(handle_ == -1) return false;
  ssize_t l = size;
  size=0;
  if(seekable_) { // check for EOF
    struct stat st;
    if(::fstat(handle_,&st) != 0) {
      FAILSYSERR("Can't get state of stream's handle: ");
    };
    off_t o = ::lseek(handle_,0,SEEK_CUR);
    if(o == (off_t)(-1)) {
      FAILSYSERR("Failed to seek in stream's handle: ");
    };
    o++;
    if(o >= st.st_size) {
      failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Failed to seek to requested position");
      return false;
    };
  };
  struct pollfd fd;
  fd.fd=handle_; fd.events=POLLIN | POLLPRI | POLLERR; fd.revents=0;
  int r = 0;
  if((r=poll(&fd,1,timeout_*1000)) != 1) {
    if(r == 0) {
      failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Timeout waiting for incoming data");
    } else {
      failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Failed while waiting for incoming data: "+StrError(errno));
    };
    return false;
  }
  if(!(fd.revents & (POLLIN | POLLPRI))) {
    failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Error in stream's handle");
    return false;
  }
  l=::read(handle_,buf,l);
  if(l == -1) {
    FAILSYSERR("Failed reading stream's handle: ");
  }
  size=l;
  if((l == 0) && (fd.revents & POLLERR)) {
    // TODO: remove because it never happens
    failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Error in stream's handle");
    return false;
  }
  return true;
}

bool PayloadStream::Put(const char* buf,Size_t size) {
  ssize_t l;
  if(handle_ == -1) return false;
  time_t start = time(NULL);
  for(;size;) {
    struct pollfd fd;
    fd.fd=handle_; fd.events=POLLOUT | POLLERR; fd.revents=0;
    int to = timeout_-(unsigned int)(time(NULL)-start);
    if(to < 0) to=0;
    int r = 0;
    if((r=poll(&fd,1,to*1000)) != 1) {
      if(r == 0) {
        failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Timeout waiting for outgoing data");
      } else {
        failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Failed while waiting for outgoing data: "+StrError(errno));
      };
      return false;
    };
    if(!(fd.revents & POLLOUT)) {
      failure_ = MCC_Status(GENERIC_ERROR,"STREAM","Error in stream's handle");
      return false;
    };
    l=::write(handle_,buf,size);
    if(l == -1) {
      FAILSYSERR("Failed writing into stream's handle: ");
    }
    buf+=l; size-=l;
  };  
  return true;
}

// ---------------------------------------------------------

bool PayloadStreamInterface::Get(std::string& buf) {
  char tbuf[1024];
  int l = sizeof(tbuf);
  bool result = Get(tbuf,l);
  buf.assign(tbuf,l);
  return result;
}

std::string PayloadStreamInterface::Get(void) {
  std::string buf;
  Get(buf);
  return buf;
}

bool PayloadStreamInterface::Get(PayloadStreamInterface& dest,int& size) {
  char tbuf[1024];
  int size_ = 0;
  bool r = false;
  while(true) {
    if(size == 0) { r = true; break; };
    int l = size;
    if((size == -1) || (l > (int)sizeof(tbuf))) l = (int)sizeof(tbuf);
    if(!Get(tbuf,l)) break;
    if(l <= 0) { r = true; break; };
    size_ += l; if(size != -1) size -= l;
    if(!dest.Put(tbuf,l)) break;
  };
  size = size_;
  return r;
}

bool PayloadStreamInterface::Put(PayloadStreamInterface& source,Size_t size) {
  char tbuf[1024];
  bool r = false;
  while(true) {
    if(size == 0) { r = true; break; };
    int l = size;
    if((size == -1) || (l > (int)sizeof(tbuf))) l = (int)sizeof(tbuf);
    if(!source.Get(tbuf,l)) break;
    if(l <= 0) { r = true; break; };
    if(!Put(tbuf,l)) break;
    if(size != -1) size -= l;
  };
  return r;
}

bool PayloadStreamInterface::Put(const std::string& buf) {
  return Put(buf.c_str(),buf.length());
}

bool PayloadStreamInterface::Put(const char* buf) {
  return Put(buf,buf?strlen(buf):0);
}

} // namespace Arc
