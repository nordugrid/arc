#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "PayloadStream.h"

namespace Arc {

PayloadStream::PayloadStream(int h):timeout_(60),handle_(h),seekable_(false) {
  struct stat st;
  if(fstat(handle_,&st) != 0) return;
  if(!(S_ISREG(st.st_mode))) return;
  seekable_=true;
  return;
}

bool PayloadStream::Get(char* buf,int& size) {
  ssize_t l = size;
  struct pollfd fd;
  size=0;
  if(handle_ == -1) return false;
  if(seekable_) { // check for EOF
    struct stat st;
    if(fstat(handle_,&st) != 0) return false;
    off_t o = lseek(handle_,0,SEEK_CUR);
    if(o == (off_t)(-1)) return false;
    o++;
    if(o >= st.st_size) return false;
  };
  fd.fd=handle_; fd.events=POLLIN | POLLPRI | POLLERR; fd.revents=0;
  if(poll(&fd,1,timeout_*1000) != 1) return false;
  if(!(fd.revents & (POLLIN | POLLPRI))) return false;
  l=::read(handle_,buf,l);
  if(l == -1) return false;
  size=l;
  if((l == 0) && (fd.revents && POLLERR)) return false;
  return true;
}

bool PayloadStream::Get(std::string& buf) {
  char tbuf[1024];
  int l = sizeof(tbuf);
  bool result = Get(tbuf,l);
  buf.assign(tbuf,l);
  return result;
}

bool PayloadStream::Put(const char* buf,int size) {
  ssize_t l;
  struct pollfd fd;
  if(handle_ == -1) return false;
  time_t start = time(NULL);
  for(;size;) {
    fd.fd=handle_; fd.events=POLLOUT | POLLERR; fd.revents=0;
    int to = timeout_-(unsigned int)(time(NULL)-start);
    if(to < 0) to=0;
    if(poll(&fd,1,to*1000) != 1) return false;
    if(!(fd.revents & POLLOUT)) return false;
    l=::write(handle_,buf,size);
    if(l == -1) return false;
    buf+=l; size-=l;
  };  
  return true;
}

} // namespace Arc
