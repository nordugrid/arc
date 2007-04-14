#ifndef __ARC_PAYLOADTCPSOCKET_H__
#define __ARC_PAYLOADTCPSOCKET_H__

#include <vector>

#include "PayloadStream.h"

namespace Arc {

/** This class extends PayloadStream with TCP socket specific features */
class PayloadTCPSocket: public PayloadStream {
 private:
  bool acquired_;
 public:
  /** Constructor - connects to TCP server at specified hostname:port */
  PayloadTCPSocket(const char* hostname,int port);
  /** Constructor - connects to TCP server at specified endpoint - hostname:port */
  PayloadTCPSocket(const std::string endpoint);
  /** Constructor - creates object of already connected socket */
  PayloadTCPSocket(int s):PayloadStream(s),acquired_(false) { };
  PayloadTCPSocket(PayloadTCPSocket& s):PayloadStream(s),acquired_(false) { };
  PayloadTCPSocket(PayloadStream& s):PayloadStream(s),acquired_(false) { };
  virtual ~PayloadTCPSocket(void);
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTCPSOCKET_H__ */

