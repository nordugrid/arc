#ifndef __ARC_SEC_SAML2SSO_ASSERTIONCONSUMERSH_H__
#define __ARC_SEC_SAML2SSO_ASSERTIONCONSUMERSH_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/message/MCCLoader.h>
#include <arc/security/SecHandler.h>

namespace ArcSec {

/// Implement the funcionality of the Service Provider in SAML2 SSO profile
//1.Launch a service (called SP Service) which will compose AuthnRequest according 
//to the IdP information sent from client side/user agent. So the SAML2SSO_ServiceProviderSH
//handler and SP Service together composes the functionality if Service Provider in 
//SAML2 SSO profile
//2.Consume the saml assertion from client side/user agent (Push model): 
//a. assertion inside soap message as WS-Security SAML token;
//b. assertion inside x509 certificate as exention. we need to parse the peer 
//x509 certificate from transport level and take out the saml assertion.
//Or contact the IdP and get back the saml assertion related to the client(Pull model)

class SAML2SSO_AssertionConsumerSH : public SecHandler {
 private:
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
  std::string ca_dir_;
  Arc::MCCLoader* SP_service_loader;

 public:
  SAML2SSO_AssertionConsumerSH(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~SAML2SSO_AssertionConsumerSH(void);
  static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg);
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_SAML2SSO_ASSERTIONCONSUMERSH_H__ */

