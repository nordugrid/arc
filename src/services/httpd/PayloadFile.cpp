#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include "PayloadFile.h"

namespace HTTPD {

PayloadFile::PayloadFile(const char* filename):handle_(-1),addr_(NULL),size_(0) {
  handle_=open(filename,O_RDONLY);
  if(handle_ == -1) return;
  struct stat st;
  if(fstat(handle_,&st) != 0) goto error;
  size_=st.st_size;
  addr_=(char*)mmap(NULL,size_,PROT_READ,MAP_SHARED,handle_,0);
  if(addr_ == MAP_FAILED) goto error;
  return;
error:
  perror("PayloadFile");
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
  if(pos >= size_) return NULL;
  return (addr_+pos);
}

char PayloadFile::operator[](int pos) const {
  if(handle_ == -1) return 0;
  if(pos >= size_) return 0;
  return addr_[pos];
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
  return size_;
}

int PayloadFile::BufferPos(unsigned int /*num*/) const {
  return 0;
}

bool PayloadFile::Truncate(unsigned int /*size*/) {
  return false;
}

} // namespace ARex
