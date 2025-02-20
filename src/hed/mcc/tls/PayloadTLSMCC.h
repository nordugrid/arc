#ifndef __ARC_PAYLOADTLSMCC_H__
#define __ARC_PAYLOADTLSMCC_H__

#include <vector>
#include <openssl/ssl.h>

#include <arc/message/PayloadStream.h>
#include <arc/message/MCC.h>
#include <arc/Logger.h>

#include "BIOMCC.h"
#include "BIOGSIMCC.h"
#include "PayloadTLSStream.h"
#include "ConfigTLSMCC.h"

namespace ArcMCCTLS {

// This class extends PayloadTLSStream with initialization procedure to 
// connect it to next MCC or Stream interface.
class PayloadTLSMCC: public PayloadTLSStream {
 private:
  /** Specifies if this object owns internal SSL objects */
  bool master_;
  /** SSL context */
  SSL_CTX* sslctx_;
  BIO* bio_;
  static int ex_data_index_;
  //PayloadTLSMCC(PayloadTLSMCC& stream);
  ConfigTLSMCC config_;
  bool StoreInstance(void);
  bool ClearInstance(void);
  // Generic purpose bit flags
  unsigned long flags_;
  bool connected_;
 public:
  /** Constructor - creates ssl object which is bound to next MCC.
    This instance must be used on client side. It obtains Stream interface
    from next MCC dynamically. */
  PayloadTLSMCC(MCCInterface* mcc, const ConfigTLSMCC& cfg, Logger& logger);
  /** Constructor - creates ssl object which is bound to stream. 
    This constructor to be used on server side. Provided stream
    is NOT destroyed in destructor. */
  PayloadTLSMCC(PayloadStreamInterface* stream, const ConfigTLSMCC& cfg, Logger& logger);
  /** Copy constructor with new logger.
    Created object shares same SSL objects but does not destroy them 
    in destructor. Main instance must be destroyed after all copied ones. */
  PayloadTLSMCC(PayloadTLSMCC& stream);
  virtual ~PayloadTLSMCC(void);
  const ConfigTLSMCC& Config(void) { return config_; };
  static PayloadTLSMCC* RetrieveInstance(X509_STORE_CTX* container);
  unsigned long Flags(void) { return flags_; };
  void Flags(unsigned long flags) { flags_=flags; };
  void SetFailure(const std::string& err);
  void SetFailure(int code = SSL_ERROR_NONE);
  operator bool(void) { return (sslctx_ != NULL); };
  bool operator!(void) { return (sslctx_ == NULL); };
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTLSMCC_H__ */

