#include <vector>

#include "PayloadStream.h"

class PayloadTCPSocket: public PayloadStream {
 public:
  PayloadTCPSocket(const char* hostname,int port);
  PayloadTCPSocket(const std::string endpoint);
  PayloadTCPSocket(int s):PayloadStream(s) { };
  virtual ~PayloadTCPSocket(void);
};


