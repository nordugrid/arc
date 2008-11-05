#ifndef __ARC_SEC_SAML2SSO_SERVICEPROVIDERSH_H__
#define __ARC_SEC_SAML2SSO_SERVICEPROVIDERSH_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/SecHandler.h>

namespace ArcSec {

/// Implement the funcionality of the user agent in SAML2 SSO profile

class SAML2SSO_ServiceProviderSH : public SecHandler {
 private:
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
  std::string ca_dir_;
  Arc::Loader* SP_service_loader;

 public:
  SAML2SSO_ServiceProviderSH(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~SAML2SSO_ServiceProviderSH(void);
  static SecHandler* get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_SAML2SSO_SERVICEPROVIDERSH_H__ */

