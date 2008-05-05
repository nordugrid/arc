#ifndef __ARC_PAYLOADTCPSOCKET_H__
#define __ARC_PAYLOADTCPSOCKET_H__

#include <vector>

#include <arc/message/PayloadStream.h>
#include <arc/Logger.h>

namespace Arc {

/** This class extends PayloadStream with TCP socket specific features */
class PayloadTCPSocket: public PayloadStream {
 private:
  int connect_socket(const char* hostname,int port);
  bool acquired_;
  Logger& logger;
 public:
  /** Constructor - connects to TCP server at specified hostname:port */
  PayloadTCPSocket(const char* hostname,int port,Logger& logger);
  /** Constructor - connects to TCP server at specified endpoint - hostname:port */
  PayloadTCPSocket(const std::string endpoint,Logger& logger);
  /** Constructor - creates object of already connected socket.
    Socket is NOT closed in destructor. */
  PayloadTCPSocket(int s,Logger& logger):
    PayloadStream(s),acquired_(false),logger(logger) { };
  /** Copy constructor - inherits socket of copied object.
    Socket is NOT closed in destructor. */
  PayloadTCPSocket(PayloadTCPSocket& s,Logger& logger):
    PayloadStream(s),acquired_(false),logger(logger) { };
  /** Copy constructor - inherits handle of copied object.
    Handle is NOT closed in destructor. */
  PayloadTCPSocket(PayloadStream& s,Logger& logger):
    PayloadStream(s),acquired_(false),logger(logger) { };
  virtual ~PayloadTCPSocket(void);
  virtual bool Get(char* buf,int& size);
  virtual bool Put(const char* buf,int size);
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTCPSOCKET_H__ */

