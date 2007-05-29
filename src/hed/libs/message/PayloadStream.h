#ifndef __ARC_PAYLOADSTREAM_H__
#define __ARC_PAYLOADSTREAM_H__

#include <unistd.h>
#include <string>

#include "Message.h"

namespace Arc {

/** Virtual interface for managing stream-like source and destination.
 It's supposed to passed through MCC chain as payload of Message.
 It must be treated by MCC as dynamic payload. 
 This class is purely virtual. */
class PayloadStreamInterface: public MessagePayload {
 public:
  PayloadStreamInterface(void) { };
  virtual ~PayloadStreamInterface(void) { };
  /** Extracts information from stream up to 'size' bytes. 
    'size' contains number of read bytes on exit.
    Returns true in case of success. */
  virtual bool Get(char* buf,int& size) = 0;
  /** Read as many as possible (sane amount) of bytes into buf. */
  virtual bool Get(std::string& buf) = 0;
  /** Read as many as possible (sane amount) of bytes. */
  virtual std::string Get(void) = 0;
  /** Push 'size' bytes from 'buf' into stream. 
    Returns true on success. */
  virtual bool Put(const char* buf,int size) = 0;
  /** Push information from 'buf' into stream. 
    Returns true on success. */
  virtual bool Put(const std::string& buf) = 0;
  /** Push null terminated information from 'buf' into stream. 
    Returns true on success. */
  virtual bool Put(const char* buf) = 0;
  /** Returns true if stream is valid. */
  virtual operator bool(void) = 0;
  /** Returns true if stream is invalid. */
  virtual bool operator!(void) = 0;
  /** Query current timeout for Get() and Put() operations. */
  virtual int Timeout(void) const = 0;
  /** Set current timeout for Get() and Put() operations. */
  virtual void Timeout(int to) = 0;
};

/** Implemetation of PayloadStreamInterface for generic POSIX handle. */
class PayloadStream: public PayloadStreamInterface {
 protected:
  int timeout_;   /** Timeout for read/write operations */
  int handle_;    /** Handle for operations */
  bool seekable_; /** true if lseek operation is applicable to open handle */
 public:
  /** Constructor. Attaches to already open handle.
    Handle is not managed by this class and must be closed by external code. */
  PayloadStream(int h = -1);
  /** Destructor. */
  virtual ~PayloadStream(void) { };
  virtual bool Get(char* buf,int& size);
  virtual bool Get(std::string& buf);
  virtual std::string Get(void) { std::string buf; Get(buf); return buf; };
  virtual bool Put(const char* buf,int size);
  virtual bool Put(const std::string& buf) { return Put(buf.c_str(),buf.length()); };
  virtual bool Put(const char* buf) { return Put(buf,buf?strlen(buf):0); };
  virtual operator bool(void) { return (handle_ != -1); };
  virtual bool operator!(void) { return (handle_ == -1); };
  virtual int Timeout(void) const { return timeout_; };
  virtual void Timeout(int to) { timeout_=to; };
  
  virtual int GetHandle(void){return handle_;};  //The method is specified for MCCTLS
};
}
#endif /* __ARC_PAYLOADSTREAM_H__ */
