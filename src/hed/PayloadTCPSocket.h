#include <vector>

#include "PayloadStream.h"

class DataPayloadTCPSocket: public DataPayloadStream {
 public:
  DataPayloadTCPSocket(const char* hostname,int port);
  DataPayloadTCPSocket(const std::string endpoint);
  DataPayloadTCPSocket(int s):DataPayloadStream(s) { };
  virtual ~DataPayloadTCPSocket(void);
};


