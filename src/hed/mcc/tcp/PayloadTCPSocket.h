#ifndef __ARC_PAYLOADTCPSOCKET_H__
#define __ARC_PAYLOADTCPSOCKET_H__

#include <vector>

#include <arc/message/PayloadStream.h>
#include <arc/Logger.h>

namespace Arc {

/** This class extends PayloadStream with TCP socket specific features */
class PayloadTCPSocket: public PayloadStreamInterface {
 private:
  int connect_socket(const char* hostname,int port);
  int handle_;
  bool acquired_;
  int timeout_;
  Logger& logger;
 public:
  /** Constructor - connects to TCP server at specified hostname:port */
  PayloadTCPSocket(const char* hostname,int port, int timeout, Logger& logger);
  /** Constructor - connects to TCP server at specified endpoint - hostname:port */
  PayloadTCPSocket(const std::string endpoint,int timeout, Logger& logger);
  /** Constructor - creates object of already connected socket.
    Socket is NOT closed in destructor. */
  PayloadTCPSocket(int s, int timeout, Logger& logger):
    handle_(s),acquired_(false),timeout_(timeout),logger(logger) { };
  /** Copy constructor - inherits socket of copied object.
    Socket is NOT closed in destructor. */
  PayloadTCPSocket(PayloadTCPSocket& s):
    handle_(s.handle_),acquired_(false),timeout_(s.timeout_),logger(s.logger) { };
  /** Copy constructor - inherits handle of copied object.
    Handle is NOT closed in destructor. */
  PayloadTCPSocket(PayloadTCPSocket& s,Logger& logger):
    handle_(s.handle_),acquired_(false),timeout_(s.timeout_),logger(logger) { };
  virtual ~PayloadTCPSocket(void);
  virtual bool Get(char* buf,int& size);
  virtual bool Get(std::string& buf);
  virtual std::string Get(void) { std::string buf; Get(buf); return buf; };
  virtual bool Put(const char* buf,Size_t size);
  virtual bool Put(const std::string& buf) { return Put(buf.c_str(),buf.length()); };
  virtual bool Put(const char* buf) { return Put(buf,buf?strlen(buf):0); };
  virtual operator bool(void) { return (handle_ != -1); };
  virtual bool operator!(void) { return (handle_ == -1); };
  virtual int Timeout(void) const { return timeout_; };
  virtual void Timeout(int to) { timeout_=to; };
  virtual Size_t Pos(void) const { return 0; };
  virtual Size_t Size(void) const { return 0; };
  virtual Size_t Limit(void) const { return 0; };
  int GetHandle() { return handle_; };
  void NoDelay(bool val);
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTCPSOCKET_H__ */

