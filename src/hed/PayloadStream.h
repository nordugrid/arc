#ifndef __ARC_PAYLOADSTREAM_H__
#define __ARC_PAYLOADSTREAM_H__

#include <unistd.h>
#include <string>

#include "Message.h"

// Virtual interface for managing stream-like source and destination
class PayloadStreamInterface: public MessagePayload {
 public:
  PayloadStreamInterface(void) { };
  virtual ~PayloadStreamInterface(void) { };
  // Extract information from stream
  virtual bool Get(char* buf,int& size) = 0;
  virtual bool Get(std::string& buf) = 0;
  virtual std::string Get(void) = 0;
  // Push information to stream
  virtual bool Put(const char* buf,int size) = 0;
  virtual bool Put(const std::string& buf) = 0;
  virtual bool Put(const char* buf) = 0;
  virtual operator bool(void) = 0;
  virtual bool operator!(void) = 0;
  virtual int Timeout(void) const = 0;
  virtual void Timeout(int to) = 0;
};

class PayloadStream: public PayloadStreamInterface {
 protected:
  int timeout_;
  int handle_;
  bool seekable_;
 public:
  PayloadStream(int h = -1);
  ~PayloadStream(void) { ::close(handle_); };
  virtual bool Get(char* buf,int& size);
  virtual bool Get(std::string& buf);
  virtual std::string Get(void) { std::string buf; Get(buf); return buf; };
  // Push information to stream
  virtual bool Put(const char* buf,int size);
  virtual bool Put(const std::string& buf) { return Put(buf.c_str(),buf.length()); };
  virtual bool Put(const char* buf) { return Put(buf,buf?strlen(buf):0); };
  virtual operator bool(void) { return (handle_ != -1); };
  virtual bool operator!(void) { return (handle_ == -1); };
  virtual int Timeout(void) const { return timeout_; };
  virtual void Timeout(int to) { timeout_=to; };
};

#endif
