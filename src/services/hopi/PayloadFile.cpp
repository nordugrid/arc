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

#include <arc/FileUtils.h>

#include "PayloadFile.h"

namespace Hopi {

PayloadBigFile::Size_t PayloadBigFile::threshold_ = 1024*1024*10; // 10MB by default

#ifndef WIN32 

PayloadFile::PayloadFile(const char* filename,Size_t start,Size_t end):handle_(-1),addr_(NULL),size_(0) {
  start_=start;
  end_=end;
  handle_=::open(filename,O_RDONLY);
  if(handle_ == -1) return;
  struct stat st;
  if(fstat(handle_,&st) != 0) goto error;
  size_=st.st_size;
  if(end_ > size_) {
    end_=size_;
  }
  if(start_ >= size_) {
    start_=size_;
    end_=start_;
    return;
  }
  if(size_ > 0) {
    addr_=(char*)mmap(NULL,size_,PROT_READ,MAP_SHARED,handle_,0);
    if(addr_ == MAP_FAILED) goto error;
  }

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
  if(pos >= end_) return NULL;
  if(pos < start_) return NULL;
  return (addr_+pos);
}

char PayloadFile::operator[](Size_t pos) const {
  if(handle_ == -1) return 0;
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
  if(addr_ == NULL) return NULL;
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

#endif

static int open_file_read(const char* filename) {
  return ::open(filename,O_RDONLY);
}

//static int open_file_write(const char* filename) {
//  return ::open(filename,O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
//}

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

Arc::MessagePayload* newFileRead(const char* filename,Arc::PayloadRawInterface::Size_t start,Arc::PayloadRawInterface::Size_t end) {
#ifndef WIN32 
  PayloadBigFile* file1 = new PayloadBigFile(filename,start,end);
  if(!*file1) { delete file1; return NULL; };
  if(file1->Size() > PayloadBigFile::Threshold()) return file1;
  PayloadFile* file2 = new PayloadFile(filename,start,end);
  if(!*file2) { delete file2; return file1; };
  delete file1;
  return file2;
#else
  PayloadBigFile* file1 = new PayloadBigFile(filename,start,end);
  if(!*file1) { delete file1; return NULL; };
  return file1;
#endif
}

} // namespace Hopi

