#ifndef __ARC_PAYLOADTCPSOCKET_H__
#define __ARC_PAYLOADTCPSOCKET_H__

#include <vector>

#include "PayloadStream.h"

class PayloadTCPSocket: public PayloadStream {
 public:
  PayloadTCPSocket(const char* hostname,int port);
  PayloadTCPSocket(const std::string endpoint);
  PayloadTCPSocket(int s):PayloadStream(s) { };
  virtual ~PayloadTCPSocket(void);
};

#endif /* __ARC_PAYLOADTCPSOCKET_H__ */

