#ifndef __ARC_PAYLOADSTREAM_H__
#define __ARC_PAYLOADSTREAM_H__

#include <string>

#include "Message.h"

namespace Arc {

/// Stream-like Payload for Message object.
/** This class is a virtual interface for managing stream-like source 
 and destination.  It's supposed to be passed through MCC chain as payload 
 of Message.  It must be treated by MCCs and Services as dynamic payload. */
class PayloadStreamInterface: virtual public MessagePayload {
 public:
  // Avoid defining size of int - just use biggest possible
  typedef signed long long int Size_t;
  PayloadStreamInterface(void) : MessagePayload() { };
  virtual ~PayloadStreamInterface(void) { };
  /** Extracts information from stream up to 'size' bytes. 
    'size' contains number of read bytes on exit.
    Returns true in case of success. */
  virtual bool Get(char* buf,int& size) = 0;
  /** Read as many as possible (sane amount) of bytes into buf.
     Implemented through call to Get(char*,int). */
  virtual bool Get(std::string& buf);
  /** Read and return as many as possible (sane amount) of bytes.
     Implemented through call to Get(std::string&). */
  virtual std::string Get(void);
  /** Read up to 'size' bytes and pass them to 'dest'.
     'size' contains number of read bytes on exit.
     If on input 'size' contains -1 then as much as possible
     is transfered.
     This method is both for convenience and for making it
     possible to have optimized implementations. */
  virtual bool Get(PayloadStreamInterface& dest,int& size);
  /** Push 'size' bytes from 'buf' into stream. 
    Returns true on success. */
  virtual bool Put(const char* buf,Size_t size) = 0;
  /** Push information from 'buf' into stream. 
    Returns true on success.
    Implemented though call to Put(const char*,Size_t). */
  virtual bool Put(const std::string& buf);
  /** Push null terminated information from 'buf' into stream. 
    Returns true on success.
    Implemented though call to Put(const char*,Size_t). */
  virtual bool Put(const char* buf);
  /** Push 'size' bytes from 'source' into stream.
     If on 'size' contains -1 then as much as possible
     is transfered.
     This method is both for convenience and for making it
     possible to have optimized implementations. */
  virtual bool Put(PayloadStreamInterface& source,Size_t size);
  /** Returns true if stream is valid. */
  virtual operator bool(void) = 0;
  /** Returns true if stream is invalid. */
  virtual bool operator!(void) = 0;
  /** Query current timeout for Get() and Put() operations. */
  virtual int Timeout(void) const = 0;
  /** Set current timeout for Get() and Put() operations. */
  virtual void Timeout(int to) = 0;
  /** Returns current position in stream if supported. */
  virtual Size_t Pos(void) const = 0;
  /** Returns size of underlying object if supported. */
  virtual Size_t Size(void) const = 0;
  /** Returns position at which stream reading will stop if supported.
     That may be not same as Size() if instance is meant to 
     provide access to only part of underlying obejct. */
  virtual Size_t Limit(void) const = 0;
};

/// POSIX handle as Payload
/** This is an implemetation of PayloadStreamInterface for generic POSIX handle. */
class PayloadStream: virtual public PayloadStreamInterface {
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
  virtual bool Put(const char* buf,Size_t size);
  virtual operator bool(void) { return (handle_ != -1); };
  virtual bool operator!(void) { return (handle_ == -1); };
  virtual int Timeout(void) const { return timeout_; };
  virtual void Timeout(int to) { timeout_=to; };
  virtual Size_t Pos(void) const { return 0; };
  virtual Size_t Size(void) const { return 0; };
  virtual Size_t Limit(void) const { return 0; };
};
}
#endif /* __ARC_PAYLOADSTREAM_H__ */
