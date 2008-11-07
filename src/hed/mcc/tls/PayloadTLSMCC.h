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

class ConfigTLSMCC {
 private:  
  std::string ca_dir_;
  std::string ca_file_;
  std::string proxy_file_;
  std::string cert_file_;
  std::string key_file_;
  bool client_authn_;
  bool globus_policy_;
  ConfigTLSMCC(void);
 public:
  ConfigTLSMCC(XMLNode cfg,bool client = false);
  const std::string& CADir(void) const { return ca_dir_; };
  const std::string& CAFile(void) const { return ca_file_; };
  const std::string& ProxyFile(void) const { return proxy_file_; };
  const std::string& CertFile(void) const { return cert_file_; };
  const std::string& KeyFile(void) const { return key_file_; };
  bool GlobusPolicy(void) const { return globus_policy_; };
  bool Set(SSL_CTX* sslctx,Logger& logger);
  bool IfClientAuthn(void) const { return client_authn_; };
};

// This class extends PayloadTLSStream with initialization procedure to 
// connect it to next MCC or Stream interface.
class PayloadTLSMCC: public PayloadTLSStream {
 private:
  /** Specifies if this object owns internal SSL objects */
  bool master_;
  /** SSL context */
  SSL_CTX* sslctx_;
  static int ex_data_index_;
  //PayloadTLSMCC(PayloadTLSMCC& stream);
  ConfigTLSMCC config_;
  bool StoreInstance(void);
  // Generic purpose bit flags
  unsigned long flags_;
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
  void Flags(unsigned long flags) { flags=flags_; };
};

} // namespace Arc 

#endif /* __ARC_PAYLOADTLSMCC_H__ */

