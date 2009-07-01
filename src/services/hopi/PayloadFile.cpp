#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef WIN32 
#include <sys/mman.h>
#endif
#include <iostream>
#include "PayloadFile.h"

namespace Hopi {

PayloadBigFile::Size_t PayloadBigFile::threshold_ = 1024*1024*10; // 10MB by default

PayloadFile::PayloadFile(const char* filename):handle_(-1),addr_(NULL),size_(0) {
  handle_=open(filename,O_RDONLY);
  if(handle_ == -1) return;
  struct stat st;
  if(fstat(handle_,&st) != 0) goto error;
  size_=st.st_size;

#ifndef WIN32 
  addr_=(char*)mmap(NULL,size_,PROT_READ,MAP_SHARED,handle_,0);
  if(addr_ == MAP_FAILED) goto error;
#else 
  goto error;
#endif

  return;
error:
  perror("PayloadFile");
  if(handle_ != -1) close(handle_);
  handle_=-1; size_=0; addr_=NULL;
  return;
}

PayloadFile::~PayloadFile(void) {

#ifndef WIN32 
  if(addr_ != NULL) munmap(addr_,size_);
#endif

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

static int open_file_read(const char* filename) {
  return ::open(filename,O_RDONLY);
}

static int open_file_write(const char* filename) {
  return ::open(filename,O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
}

PayloadBigFile::PayloadBigFile(const char* filename):
                       PayloadStream(open_file_read(filename)) {
  seekable_ = false;
}

PayloadBigFile::PayloadBigFile(const char* filename,Size_t size):
                       PayloadStream(open_file_write(filename)){
  seekable_ = false;
}

PayloadBigFile::~PayloadBigFile(void) {
  if(handle_ != -1) ::close(handle_);
}

Arc::PayloadStream::Size_t PayloadBigFile::Pos(void) const {
  if(handle_ == -1) return 0;
  return lseek(handle_,0,SEEK_CUR);
}

Arc::PayloadStream::Size_t PayloadBigFile::Size(void) const {
  if(handle_ == -1) return 0;
  struct stat st;
  if(fstat(handle_,&st) != 0) return 0;
  return st.st_size;
}

Arc::MessagePayload* newFileRead(const char* filename) {
  PayloadBigFile* file1 = new PayloadBigFile(filename);
  if(!*file1) { delete file1; return NULL; };
#ifndef WIN32 
  if(file1->Size() > PayloadBigFile::Threshold()) return file1;
  PayloadFile* file2 = new PayloadFile(filename);
  if(!*file2) { delete file2; return file1; };
  delete file1;
  return file2;
#else
  return file1;
#endif
}

} // namespace ARex

