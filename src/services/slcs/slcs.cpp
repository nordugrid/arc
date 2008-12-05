#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/Thread.h>

#include "slcs.h"

namespace ArcSec {

static Arc::LogStream logcerr(std::cerr);

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    return new Service_SLCS((Arc::Config*)(*srvarg));
}

Arc::MCC_Status Service_SLCS::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing slcs(short-lived certificate) request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Service_SLCS::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");

  if(!ProcessSecHandlers(inmsg,"incoming")) {
    logger_.msg(Arc::ERROR, "Security Handlers processing failed");
    return Arc::MCC_Status();
  };

  //Get identity-related information from saml assertion which has been put 
  //as MessageAuthContext by SPService on the same connection as this slcs service
  std::string identity;
  std::string ou, cn;
  std::string saml_assertion_str; 
  {
    Arc::SecAttr* sattr = inmsg.Auth()->get("SAMLAssertion");
    Arc::XMLNode saml_assertion_nd;
    if(sattr->Export(Arc::SecAttr::SAML, saml_assertion_nd)) {
      saml_assertion_nd.GetXML(saml_assertion_str);
      //The following code is IdP implementation specific. So for
      //different types of saml assertion got from IdP, different 
      //ways should be implemented here to get the identity-related
      //information. 
      //Probably, a specific sec handler is needed here for the identity parsing.
      Arc::XMLNode attr_statement = saml_assertion_nd["AttributeStatement"];
      for(int i=0;; i++) {
        Arc::XMLNode cnd = attr_statement.Child(i);
        if(!cnd) break;
        if((std::string)(cnd.Attribute("FriendlyName")) == "eduPersonPrincipalName") {
          identity = (std::string)(cnd["AttributeValue"]);
          std::size_t pos;
          pos = identity.find("@");
          if(pos != std::string::npos) {
            cn = identity.substr(0, pos);
            ou = identity.substr(pos+1);
          };
        };
      };
    };
  };

  // Identify which of served endpoints request is for.
  // SLCS can only accept POST method
  if(method == "POST") {
    logger.msg(Arc::DEBUG, "process: POST");
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };
    // Analyzing request
    Arc::XMLNode request = (*inpayload)["GetSLCSCertificateRequest"];
    if(!request) {
      logger.msg(Arc::ERROR, "soap body does not include any request node");
      return make_soap_fault(outmsg);
    };
    {
      std::string req_xml;
      request.GetXML(req_xml);
      logger.msg(Arc::DEBUG, "Request: %s",req_xml);
    };

    Arc::XMLNode x509_req_nd = request["X509Request"];
    if(!x509_req_nd) {
      logger.msg(Arc::ERROR, "There is no X509Request node in the request message");
      return make_soap_fault(outmsg);
    };
    //If no lifetime specified from request, default lifetime (12hours) will be used
    Arc::XMLNode lifetime_nd = request["LifeTime"];
    
    std::string x509_req = (std::string)x509_req_nd;
    std::string lifetime;
    if((bool)lifetime_nd) lifetime = (std::string)lifetime_nd;

    //Inquire the x509 request 
    Arc::Credential eec;
    eec.InquireRequest(x509_req, true);

    // TODO: add extensions, e.g. saml extension parsed from SPService
    /*
    std::string ext_data("test extension data");
    if(!(eec.AddExtension("1.2.3.4", ext_data))) {
      std::cout<<"Failed to add an extension to certificate"<<std::endl;
    }
    */
    if(!(eec.AddExtension("1.3.6.1.4.1.3536.1.1.1.10", saml_assertion_str))) { //Need to investigate the OID 
      std::cout<<"Failed to add saml extension to certificate"<<std::endl;
    }

    //std::string dn("/O=KnowARC/OU=UIO/CN=Test001"); 
    //TODO: compose the base name from configuration and name from saml assertion?
    std::string dn("/O=KnowARC/OU=");
    dn.append(ou).append("/CN=").append(cn);
    logger_.msg(Arc::INFO, "Composed DN: %s",dn.c_str());


    //Sign the certificate
    std::string x509_cert;
    ca_credential_->SignEECRequest(&eec, dn, x509_cert);

    //Compose response message
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
    Arc::XMLNode response = outpayload->NewChild("slcs:GetSLCSCertificateResponse");
    Arc::XMLNode x509_res_nd = response.NewChild("slcs:X509Certificate");
    x509_res_nd = x509_cert;
    Arc::XMLNode ca_res_nd = response.NewChild("slcs:CACertificate");
    std::string ca_str;
    ca_credential_->OutputCertificate(ca_str);
    ca_res_nd = ca_str;

    outmsg.Payload(outpayload);

    if(!ProcessSecHandlers(outmsg,"outgoing")) {
      logger_.msg(Arc::ERROR, "Security Handlers processing failed");
      delete outmsg.Payload(NULL);
      return Arc::MCC_Status();
    };

    return Arc::MCC_Status(Arc::STATUS_OK);
  }
  else {
    delete inmsg.Payload();
    logger.msg(Arc::DEBUG, "process: %s: not supported",method);
    return Arc::MCC_Status();
  }
  return Arc::MCC_Status();
}

Service_SLCS::Service_SLCS(Arc::Config *cfg):Service(cfg), 
    logger_(Arc::Logger::rootLogger, "SLCS_Service"), ca_credential_(NULL) {

  logger_.addDestination(logcerr);
  ns_["slcs"]="http://www.nordugrid.org/schemas/slcs";

  std::string CAcert, CAkey, CAserial;
  CAcert = (std::string)((*cfg)["CACertificate"]);
  CAkey = (std::string)((*cfg)["CAKey"]);
  CAserial = (std::string)((*cfg)["CASerial"]);
  ca_credential_ = new Arc::Credential(CAcert, CAkey, CAserial, 0, "", "");
}

Service_SLCS::~Service_SLCS(void) {
  if(ca_credential_) delete ca_credential_;
}

} // namespace ArcSec

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "slcs.service", "HED:SERVICE", 0, &ArcSec::get_service },
    { NULL, NULL, 0, NULL }
};

