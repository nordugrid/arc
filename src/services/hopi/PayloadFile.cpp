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

namespace Hopi {

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

char* PayloadFile::Content(Size_t pos) {
  if(handle_ == -1) return NULL;
  if(pos >= size_) return NULL;
  return (addr_+pos);
}

char PayloadFile::operator[](Size_t pos) const {
  if(handle_ == -1) return 0;
  if(pos >= size_) return 0;
  return addr_[pos];
}

PayloadFile::Size_t PayloadFile::Size(void) const {
  return size_;
}

char* PayloadFile::Insert(Size_t /*pos*/,Size_t /*size*/) {
  // Not supported
  return NULL;
}

char* PayloadFile::Insert(const char*,Size_t /*pos*/,Size_t /*size*/) {
  // Not supported
  return NULL;
}

char* PayloadFile::Buffer(unsigned int num) {
  if(handle_ == -1) return NULL;
  if(num>0) return NULL;
  return addr_;
}

PayloadFile::Size_t PayloadFile::BufferSize(unsigned int num) const {
  if(handle_ == -1) return 0;
  if(num>0) return 0;
  return size_;
}

PayloadFile::Size_t PayloadFile::BufferPos(unsigned int num) const {
  if(num == 0) return 0;
  return size_;
}

bool PayloadFile::Truncate(Size_t /*size*/) {
  return false;
}

} // namespace ARex
