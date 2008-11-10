#ifndef __ARC_SEC_SAML2SSO_USERAGENTSH_H__
#define __ARC_SEC_SAML2SSO_USERAGENTSH_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/SecHandler.h>

namespace ArcSec {

/// Implement the funcionality of the user agent in SAML2 SSO profile
//Beside the client/service interaction/communication functioned by the main 
//message chain, the saml2ssouseragent.handler (embedded in http component or soap 
//component) will launch other intetactions/communications with the peer security
//handler (SP Service), and also with the IdP service, in order to get back the 
//saml assertion; saml2ssoserviceprovider.handler on the service side will consume
//the saml assertion.
//Note: In order to simplify the IdP discovery problem, here we simply let the 
//user agent send the IdP name to service side. So the functionality of WRYF
//(where are you from) or Discovery Service is avoided.

class SAML2SSO_UserAgentSH : public SecHandler {
 private:
  std::string cert_file_;
  std::string privkey_file_;
  std::string ca_file_;
  std::string ca_dir_;

 public:
  SAML2SSO_UserAgentSH(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~SAML2SSO_UserAgentSH(void);
  static SecHandler* get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_SAML2SSO_USERAGENTSH_H__ */

