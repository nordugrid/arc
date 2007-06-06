#ifndef __ARC_PAYLOADTLSSOCKET_H__
#define __ARC_PAYLOADTLSSOCKET_H__

#include <vector>
#include <openssl/ssl.h>

#include "PayloadTLSStream.h"
#include "../../libs/message/PayloadStream.h"

namespace Arc {
// This class extends PayloadTLSStream with TLS socket specific features
class PayloadTLSSocket: public PayloadTLSStream {
 private:
  SSL_CTX* sslctx_;
  bool client_;  //A flag indicates whether the stream is from client side or server side.  client==true, means ssl client;  client==false, means ssl server
 public:
  /** Constructor - creat ssl object which is binded into socket object */
  PayloadTLSSocket(int s, SSL_CTX* ctx, bool client);
  PayloadTLSSocket(PayloadStream& s, SSL_CTX* ctx, bool client);
  
  virtual ~PayloadTLSSocket(void);
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTLSSOCKET_H__ */

