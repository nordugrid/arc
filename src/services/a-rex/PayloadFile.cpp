#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include "PayloadFile.h"

namespace ARex {

PayloadFile::PayloadFile(const char* filename):handle_(-1),addr_(NULL),size_(0) {
std::cerr<<"PayloadFile: filename="<<filename<<std::endl;
  handle_=open(filename,O_RDONLY);
  if(handle_ == -1) return;
std::cerr<<"PayloadFile: open passed"<<std::endl;
  struct stat st;
  if(fstat(handle_,&st) != 0) goto error;
std::cerr<<"PayloadFile: fstat passed"<<std::endl;
  size_=st.st_size;
  addr_=(char*)mmap(NULL,size_,PROT_READ,MAP_SHARED,handle_,0);
  if(addr_ == MAP_FAILED) goto error;
std::cerr<<"PayloadFile: mmap passed"<<std::endl;
  return;
error:
perror("PayloadFile");
  if(handle_ != -1) close(handle_);
  handle_=-1; size_=0; addr_=NULL;
  return;
}

PayloadFile::~PayloadFile(void) {
std::cerr<<"~PayloadFile"<<std::endl;
  if(addr_ != NULL) munmap(addr_,size_);
  close(handle_);
  handle_=-1; size_=0; addr_=NULL;
  return;
}

char* PayloadFile::Content(int pos) {
std::cerr<<"PayloadFile: Content: "<<pos<<std::endl;
  if(handle_ == -1) return NULL;
  if(pos >= size_) return NULL;
std::cerr<<"PayloadFile: Content"<<std::endl;
  return (addr_+pos);
}

char PayloadFile::operator[](int pos) const {
  if(handle_ == -1) return 0;
  if(pos >= size_) return 0;
  return addr_[pos];
}

int PayloadFile::Size(void) const {
std::cerr<<"PayloadFile: Size"<<std::endl;
  return size_;
}

char* PayloadFile::Insert(int /*pos*/,int /*size*/) {
std::cerr<<"PayloadFile: Insert"<<std::endl;
  // Not supported
  return NULL;
}

char* PayloadFile::Insert(const char*,int /*pos*/,int /*size*/) {
std::cerr<<"PayloadFile: Insert"<<std::endl;
  // Not supported
  return NULL;
}

char* PayloadFile::Buffer(unsigned int num) {
std::cerr<<"PayloadFile: Buffer: "<<num<<std::endl;
  if(handle_ == -1) return NULL;
  if(num>0) return NULL;
std::cerr<<"PayloadFile: Buffer"<<std::endl;
  return addr_;
}

int PayloadFile::BufferSize(unsigned int num) const {
std::cerr<<"PayloadFile: BufferSize: "<<num<<std::endl;
  if(handle_ == -1) return 0;
  if(num>0) return 0;
std::cerr<<"PayloadFile: BufferSize"<<std::endl;
  return size_;
}

int PayloadFile::BufferPos(unsigned int /*num*/) const {
std::cerr<<"PayloadFile: BufferPos"<<std::endl;
  return 0;
}

bool PayloadFile::Truncate(unsigned int /*size*/) {
std::cerr<<"PayloadFile: Truncate"<<std::endl;
  return false;
}

} // namespace ARex
