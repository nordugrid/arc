#ifndef __ARC_PAYLOADFILE_H__
#define __ARC_PAYLOADFILE_H__

#include <vector>

#include <arc/message/PayloadRaw.h>

namespace ARex {

/** Implementation of PayloadRawInterface which provides access to ordinary file.
  Currently only read-only mode is supported. */
class PayloadFile: public Arc::PayloadRawInterface {
 protected:
  /* TODO: use system-independent file access */
  int handle_;
  char* addr_;
  size_t size_; 
  size_t start_;
  size_t end_;
 public:
  /** Creates object associated with file for reading from it */
  PayloadFile(const char* filename,size_t start = 0,size_t end = (size_t)(-1));
  /** Creates object associated with file handle for reading or writing to it */
  PayloadFile(int handle,size_t start = 0,size_t end = (size_t)(-1));
  /** Creates object associated with file for writing into it.
    Use size=-1 for undefined size. */
  PayloadFile(const char* filename,int size);
  virtual ~PayloadFile(void);
  virtual char operator[](Size_t pos) const;
  virtual char* Content(Size_t pos = -1);
  virtual Size_t Size(void) const;
  virtual char* Insert(Size_t pos = 0,Size_t size = 0);
  virtual char* Insert(const char* s,Size_t pos = 0,Size_t size = -1);
  virtual char* Buffer(unsigned int num);
  virtual Size_t BufferSize(unsigned int num) const;
  virtual Size_t BufferPos(unsigned int num) const;
  virtual bool Truncate(Size_t size);

  operator bool(void) { return (handle_ != -1); };
  bool operator!(void) { return (handle_ == -1); };
};

} // namespace ARex

#endif /* __ARC_PAYLOADFILE_H__ */
