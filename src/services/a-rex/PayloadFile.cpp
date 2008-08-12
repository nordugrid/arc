#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <arc/Logger.h>
#include "PayloadFile.h"

namespace ARex {

PayloadFile::PayloadFile(const char* filename,size_t start,size_t end):handle_(-1),addr_(NULL),size_(0),start_(start),end_(end) {
  handle_=open(filename,O_RDONLY);
  if(handle_ == -1) return;
  struct stat st;
  if(fstat(handle_,&st) != 0) goto error;
  size_=st.st_size;
  if((end_ == (size_t)(-1)) || (end_ > size_)) end_=size_;
  if(start_>end_) start_=end_;
  if(end_ > start_) {
    addr_=(char*)mmap(NULL,end_-start_,PROT_READ,MAP_SHARED,handle_,start_);
    if(addr_ == MAP_FAILED) goto error;
  };
  return;
error:
  char errbuf[256];
  errbuf[0]=0;
#if HAVE_DECL_STRERROR_R == 1
  strerror_r(errno,errbuf,sizeof(errbuf)-1);
  errbuf[sizeof(errbuf)-1]=0;
  Arc::Logger::rootLogger.msg(Arc::ERROR,"%s",errbuf);
#endif
  if(handle_ != -1) close(handle_);
  handle_=-1; size_=0; addr_=NULL;
  return;
}

PayloadFile::~PayloadFile(void) {
  if(addr_ != NULL) munmap(addr_,size_);
  close(handle_);
  handle_=-1; size_=0; addr_=NULL;
  return;
}

char* PayloadFile::Content(int pos) {
  if(handle_ == -1) return NULL;
  if(pos >= end_) return NULL;
  if(pos < start_) return NULL;
  return (addr_+(pos-start_));
}

char PayloadFile::operator[](int pos) const {
  if(handle_ == -1) return 0;
  if(pos >= end_) return 0;
  if(pos < start_) return 0;
  return addr_[pos-start_];
}

int PayloadFile::Size(void) const {
  return size_;
}

char* PayloadFile::Insert(int /*pos*/,int /*size*/) {
  // Not supported
  return NULL;
}

char* PayloadFile::Insert(const char*,int /*pos*/,int /*size*/) {
  // Not supported
  return NULL;
}

char* PayloadFile::Buffer(unsigned int num) {
  if(handle_ == -1) return NULL;
  if(num>0) return NULL;
  return addr_;
}

int PayloadFile::BufferSize(unsigned int num) const {
  if(handle_ == -1) return 0;
  if(num>0) return 0;
  return (end_-start_);
}

int PayloadFile::BufferPos(unsigned int num) const {
  if(num == 0) return start_;
  return end_;
}

bool PayloadFile::Truncate(unsigned int /*size*/) {
  return false;
}

} // namespace ARex
