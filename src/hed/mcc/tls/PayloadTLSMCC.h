#ifndef __ARC_PAYLOADTLSMCC_H__
#define __ARC_PAYLOADTLSMCC_H__

#include <vector>
#include <openssl/ssl.h>

#include "PayloadTLSStream.h"
#include <arc/message/PayloadStream.h>
#include <arc/message/MCC.h>
#include "BIOMCC.h"
#include <arc/Logger.h>

namespace Arc {
// This class extends PayloadTLSStream with initialization procedure to 
// connect it to next MCC or Stream interface.
class PayloadTLSMCC: public PayloadTLSStream {
 private:
  /** Specifies if this object own internal SSL object */
  bool master_;
  /** SSL context */
  SSL_CTX* sslctx_;
  Logger& logger;
  PayloadTLSMCC(PayloadTLSMCC& stream);
 public:
  /** Constructor - creates ssl object which is bound to next MCC.
    This instance must be used on client side. It obtains Stream interface
    from next MCC dynamically. */
  PayloadTLSMCC(MCCInterface* mcc, SSL_CTX* ctx, const std::string& cert_file, const std::string& key_file, Logger& logger);
  /** Constructor - creates ssl object which is bound to stream. 
    This constructor to be used on server side. Provided stream
    is NOT destroyed in destructor. */
  PayloadTLSMCC(PayloadStreamInterface* stream, SSL_CTX* ctx, const std::string& cert_file, const std::string& key_file, Logger& logger);
  /** Copy constructor with new logger.
    Created object shares same SSL object but does not destroy it in destructor.
    Main instance must be destroyed after all copied ones. */
  PayloadTLSMCC(PayloadTLSMCC& stream, Logger& logger);
  virtual ~PayloadTLSMCC(void);
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTLSMCC_H__ */

