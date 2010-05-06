#ifndef __ARC_CONFIGTLSMCC_H__
#define __ARC_CONFIGTLSMCC_H__

#include <vector>

#include <openssl/ssl.h>

#include <arc/XMLNode.h>
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
  bool globus_gsi_;
  enum {
    tls_handshake,
    ssl3_handshake
  } handshake_;
  std::vector<std::string> vomscert_trust_dn_;
  ConfigTLSMCC(void);
 public:
  ConfigTLSMCC(XMLNode cfg,Logger& logger,bool client = false);
  const std::string& CADir(void) const { return ca_dir_; };
  const std::string& CAFile(void) const { return ca_file_; };
  const std::string& ProxyFile(void) const { return proxy_file_; };
  const std::string& CertFile(void) const { return cert_file_; };
  const std::string& KeyFile(void) const { return key_file_; };
  bool GlobusPolicy(void) const { return globus_policy_; };
  bool GlobusGSI(void) const { return globus_gsi_; };
  const std::vector<std::string>& VOMSCertTrustDN(void) { return vomscert_trust_dn_; };
  bool Set(SSL_CTX* sslctx,Logger& logger);
  bool IfClientAuthn(void) const { return client_authn_; };
  bool IfTLSHandshake(void) const { return handshake_ == tls_handshake; };
  bool IfSSLv3Handshake(void) const { return handshake_ == ssl3_handshake; };
};

} // namespace Arc 

#endif /* __ARC_CONFIGTLSMCC_H__ */

