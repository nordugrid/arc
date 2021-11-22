#ifndef __ARC_CONFIGTLSMCC_H__
#define __ARC_CONFIGTLSMCC_H__

#include <vector>

#include <openssl/ssl.h>

#include <arc/XMLNode.h>
#include <arc/Logger.h>

namespace ArcMCCTLS {

using namespace Arc;

class ConfigTLSMCC {
 private:  
  static Logger logger;
  std::string ca_dir_;
  std::string ca_file_;
  std::string voms_dir_;
  std::string proxy_file_;
  std::string cert_file_;
  std::string key_file_;
  std::string credential_;
  bool client_authn_;
  bool globus_policy_;
  bool globus_gsi_;
  bool globusio_gsi_;
  enum {
    tls_handshake, // default
    ssl3_handshake,
    tls10_handshake,
    tls11_handshake,
    tls12_handshake,
    dtls_handshake,
    dtls10_handshake,
    dtls12_handshake,
  } handshake_;
  enum {
    relaxed_voms,
    standard_voms,
    strict_voms,
    noerrors_voms
  } voms_processing_;
  std::vector<std::string> vomscert_trust_dn_;
  std::string cipher_list_;
  std::string hostname_;
  std::string protocols_;
  std::string protocol_;
  std::string failure_;
  ConfigTLSMCC(void);
 public:
  ConfigTLSMCC(XMLNode cfg,bool client = false);
  const std::string& CADir(void) const { return ca_dir_; };
  const std::string& CAFile(void) const { return ca_file_; };
  const std::string& VOMSDir(void) const { return voms_dir_; };
  const std::string& ProxyFile(void) const { return proxy_file_; };
  const std::string& CertFile(void) const { return cert_file_; };
  const std::string& KeyFile(void) const { return key_file_; };
  bool GlobusPolicy(void) const { return globus_policy_; };
  bool GlobusGSI(void) const { return globus_gsi_; };
  bool GlobusIOGSI(void) const { return globusio_gsi_; };
  const std::vector<std::string>& VOMSCertTrustDN(void) { return vomscert_trust_dn_; };
  bool Set(SSL_CTX* sslctx);
  bool IfClientAuthn(void) const { return client_authn_; };
  bool IfTLSHandshake(void) const { return handshake_ == tls_handshake; };
  bool IfSSLv3Handshake(void) const { return handshake_ == ssl3_handshake; };
  bool IfTLSv1Handshake(void) const { return handshake_ == tls10_handshake; };
  bool IfTLSv11Handshake(void) const { return handshake_ == tls11_handshake; };
  bool IfTLSv12Handshake(void) const { return handshake_ == tls12_handshake; };
  bool IfDTLSHandshake(void) const { return handshake_ == dtls_handshake; };
  bool IfDTLSv1Handshake(void) const { return handshake_ == dtls10_handshake; };
  bool IfDTLSv12Handshake(void) const { return handshake_ == dtls12_handshake; };
  bool IfCheckVOMSCritical(void) const { return (voms_processing_ != relaxed_voms); };
  bool IfFailOnVOMSParsing(void) const { return (voms_processing_ == noerrors_voms) || (voms_processing_ == strict_voms); };
  bool IfFailOnVOMSInvalid(void) const { return (voms_processing_ == noerrors_voms); };
  const std::string& Hostname() const { return hostname_; };
  const std::string& Failure(void) { return failure_; };
  static std::string HandleError(int code = SSL_ERROR_NONE);
  static void ClearError(void);
};

} // namespace Arc 

#endif /* __ARC_CONFIGTLSMCC_H__ */

