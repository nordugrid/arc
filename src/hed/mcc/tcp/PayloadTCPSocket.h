#ifndef __ARC_PAYLOADTCPSOCKET_H__
#define __ARC_PAYLOADTCPSOCKET_H__

#include <vector>

#include "../../libs/message/PayloadStream.h"
#include "../../../libs/common/Logger.h"

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
  /** Constructor - creates object of already connected socket */
  PayloadTCPSocket(int s,Logger& logger):
    PayloadStream(s),acquired_(false),logger(logger) { };
  PayloadTCPSocket(PayloadTCPSocket& s,Logger& logger):
    PayloadStream(s),acquired_(false),logger(logger) { };
  PayloadTCPSocket(PayloadStream& s,Logger& logger):
    PayloadStream(s),acquired_(false),logger(logger) { };
  virtual ~PayloadTCPSocket(void);
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTCPSOCKET_H__ */

