#ifndef __ARC_SERVICE_SLCS_H__
#define __ARC_SERVICE_SLCS_H__

#include <arc/infosys/RegisteredService.h>
#include <arc/credential/Credential.h>
#include <arc/infosys/InformationInterface.h>

namespace ArcSec {

///A Service which signs the short-lived certificate; it accepts the certificate signing request (CSR) 
///from from client side through soap, signs a short-lived certificate and sends back through soap.
///This service is supposed to be deployed together with the SPService and saml2sso.serviceprovider
///handler, in order to sign certificate based on the authentication result from saml2sso profile.
///Also the saml attribute (inside the saml assertion from saml2sso profile) will be put into the 
///signed short-lived certificate.
///By deploying this service together with SPService and saml2sso.serviceprovider handler, we can get
///the convertion from username/password ------> x509 certificate.
//
class Service_SLCS: public Arc::RegisteredService {
 protected:
  Arc::NS ns_;
  Arc::Logger logger_;
  std::string endpoint_;
  std::string expiration_;
  Arc::InformationContainer infodoc;
  Arc::MCC_Status make_soap_fault(Arc::Message& outmsg);
 public:
  Service_SLCS(Arc::Config *cfg);
  virtual ~Service_SLCS(void);
  virtual Arc::MCC_Status process(Arc::Message& inmsg,Arc::Message& outmsg);
  bool RegistrationCollector(Arc::XMLNode &doc);
 private:
  Arc::Credential* ca_credential_;
};

} // namespace ArcSec

#endif

