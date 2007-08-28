#ifndef __ARC_PAYLOADFILE_H__
#define __ARC_PAYLOADFILE_H__

#include <vector>

#include <message/PayloadRaw.h>

namespace ARex {

/** Implementation of PayloadRawInterface whic provides access to aordinary file.
  Current read-only mode is supported. */
class PayloadFile: public Arc::PayloadRawInterface {
 protected:
  /* TODO: use system-independent file access */
  int handle_;
  char* addr_;
  size_t size_; 
 public:
  /** Creates object associated with file for reading from it */
  PayloadFile(const char* filename);
  /** Creates object associated with file for writing into it.
    Use size=-1 for undefined size. */
  PayloadFile(const char* filename,int size);
  virtual ~PayloadFile(void);
  virtual char operator[](int pos) const;
  virtual char* Content(int pos = -1);
  virtual int Size(void) const;
  virtual char* Insert(int pos = 0,int size = 0);
  virtual char* Insert(const char* s,int pos = 0,int size = 0);
  virtual char* Buffer(unsigned int num);
  virtual int BufferSize(unsigned int num) const;
  virtual int BufferPos(unsigned int num) const;
  virtual bool Truncate(unsigned int size);

  operator bool(void) { return (handle_ != -1); };
  bool operator!(void) { return (handle_ == -1); };
};

} // namespace ARex

#endif /* __ARC_PAYLOADFILE_H__ */
