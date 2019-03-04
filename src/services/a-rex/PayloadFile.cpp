#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <arc/FileUtils.h>
#include "PayloadFile.h"

namespace ARex {

PayloadBigFile::Size_t PayloadBigFile::threshold_ = 1024*1024*10; // 10MB by default

PayloadFile::PayloadFile(const char* filename,Size_t start,Size_t end) {
  handle_=::open(filename,O_RDONLY);
  SetRead(handle_,start,end);
}

PayloadFile::PayloadFile(int h,Size_t start,Size_t end) {
  SetRead(h,start,end);
}

void PayloadFile::SetRead(int h,Size_t start,Size_t end) {
  handle_=h;
  start_=start;
  end_=end;
  addr_=(char*)MAP_FAILED;
  size_=0;
  if(handle_ == -1) return;
  struct stat st;
  if(fstat(handle_,&st) != 0) goto error;
  size_=st.st_size;
  if((end_ == ((off_t)-1)) || (end_ > size_)) {
    end_=size_;
  }
  if(start_ >= size_) {
    start_=size_;
    end_=start_;
    return;
  }

  if(size_ > 0) {
    addr_=(char*)mmap(NULL,size_,PROT_READ,MAP_SHARED,handle_,0);
    if(addr_ == (char*)MAP_FAILED) goto error;
  }

  return;
error:
  perror("PayloadFile");
  if(handle_ != -1) ::close(handle_);
  handle_=-1; size_=0; addr_=(char*)MAP_FAILED;
  return;
}

PayloadFile::~PayloadFile(void) {
  if(addr_ != (char*)MAP_FAILED) munmap(addr_,size_);
  if(handle_ != -1) ::close(handle_);
  handle_=-1; size_=0; addr_=(char*)MAP_FAILED;
  return;
}

char* PayloadFile::Content(Size_t pos) {
  if(handle_ == -1) return NULL;
  if(addr_ == (char*)MAP_FAILED) return NULL;
  if(pos >= end_) return NULL;
  if(pos < start_) return NULL;
  return (addr_+pos);
}

char PayloadFile::operator[](Size_t pos) const {
  if(handle_ == -1) return 0;
  if(addr_ == (char*)MAP_FAILED) return 0;
  if(pos >= end_) return 0;
  if(pos < start_) return 0;
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
  if(addr_ == (char*)MAP_FAILED) return NULL;
  return addr_+start_;
}

PayloadFile::Size_t PayloadFile::BufferSize(unsigned int num) const {
  if(handle_ == -1) return 0;
  if(num>0) return 0;
  return (end_-start_);
}

PayloadFile::Size_t PayloadFile::BufferPos(unsigned int num) const {
  if(num == 0) return start_;
  return end_;
}

bool PayloadFile::Truncate(Size_t /*size*/) {
  // Not supported
  return false;
}

static int open_file_read(const char* filename) {
  return ::open(filename,O_RDONLY);
}

//static int open_file_write(const char* filename) {
//  return ::open(filename,O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
//}

PayloadBigFile::PayloadBigFile(int h,Size_t start,Size_t end):
                       PayloadStream(h) {
  seekable_ = false;
  if(handle_ == -1) return;
  ::lseek(handle_,start,SEEK_SET);
  limit_ = end;
}

PayloadBigFile::PayloadBigFile(const char* filename,Size_t start,Size_t end):
                       PayloadStream(open_file_read(filename)) {
  seekable_ = false;
  if(handle_ == -1) return;
  ::lseek(handle_,start,SEEK_SET);
  limit_ = end;
}

//PayloadBigFile::PayloadBigFile(const char* filename,Size_t size):
//                       PayloadStream(open_file_write(filename)){
//  seekable_ = false;
//}

PayloadBigFile::~PayloadBigFile(void) {
  if(handle_ != -1) ::close(handle_);
}

Arc::PayloadStream::Size_t PayloadBigFile::Pos(void) const {
  if(handle_ == -1) return 0;
  return ::lseek(handle_,0,SEEK_CUR);
}

Arc::PayloadStream::Size_t PayloadBigFile::Size(void) const {
  if(handle_ == -1) return 0;
  struct stat st;
  if(fstat(handle_,&st) != 0) return 0;
  return st.st_size;
}

Arc::PayloadStream::Size_t PayloadBigFile::Limit(void) const {
  Size_t s = Size();
  if((limit_ == (off_t)(-1)) || (limit_ > s)) return s;
  return limit_;
}

bool PayloadBigFile::Get(char* buf,int& size) {
  if(handle_ == -1) return false;
  if(limit_ == (off_t)(-1)) return PayloadStream::Get(buf,size);
  Size_t cpos = Pos();
  if(cpos >= limit_) {
    size=0; return false;
  }
  if((cpos+size) > limit_) size=limit_-cpos;
  return PayloadStream::Get(buf,size);
}

PayloadFAFile::PayloadFAFile(Arc::FileAccess* h,Size_t start,Size_t end) {
  handle_ = h;
  if(handle_ == NULL) return;
  handle_->fa_lseek(start,SEEK_SET);
  limit_ = end;
}

PayloadFAFile::~PayloadFAFile(void) {
  if(handle_ != NULL) {
    handle_->fa_close();
    Arc::FileAccess::Release(handle_);
  };
}

Arc::PayloadStream::Size_t PayloadFAFile::Pos(void) const {
  if(handle_ == NULL) return 0;
  return handle_->fa_lseek(0,SEEK_CUR);
}

Arc::PayloadStream::Size_t PayloadFAFile::Size(void) const {
  if(handle_ == NULL) return 0;
  struct stat st;
  if(!handle_->fa_fstat(st)) return 0;
  return st.st_size;
}

Arc::PayloadStream::Size_t PayloadFAFile::Limit(void) const {
  Size_t s = Size();
  if((limit_ == (off_t)(-1)) || (limit_ > s)) return s;
  return limit_;
}

bool PayloadFAFile::Get(char* buf,int& size) {
  if(handle_ == NULL) return false;
  if(limit_ != (off_t)(-1)) {
    Size_t cpos = Pos();
    if(cpos >= limit_) { size=0; return false; }
    if((cpos+size) > limit_) size=limit_-cpos;
  };
  ssize_t l = handle_->fa_read(buf,size);
  if(l <= 0) { size=0; return false; }
  size = (int)l;
  return true;
}

Arc::MessagePayload* newFileRead(const char* filename,Arc::PayloadRawInterface::Size_t start,Arc::PayloadRawInterface::Size_t end) {
  int h = open_file_read(filename);
  return newFileRead(h,start,end);
}

Arc::MessagePayload* newFileRead(int h,Arc::PayloadRawInterface::Size_t start,Arc::PayloadRawInterface::Size_t end) {
  struct stat st;
  if(fstat(h,&st) != 0) return NULL;
  if(st.st_size > PayloadBigFile::Threshold()) {
    PayloadBigFile* f = new PayloadBigFile(h,start,end);
    if(!*f) { delete f; return NULL; };
    return f;
  }
  PayloadFile* f = new PayloadFile(h,start,end);
  if(!*f) { delete f; return NULL; };
  return f;
}

Arc::MessagePayload* newFileRead(Arc::FileAccess* h,Arc::PayloadRawInterface::Size_t start,Arc::PayloadRawInterface::Size_t end) {
  PayloadFAFile* f = new PayloadFAFile(h,start,end);
  return f;
}

} // namespace ARex

