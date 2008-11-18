#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include <arc/loader/Loader.h>
#include <arc/loader/ClassLoader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/loader/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/Thread.h>

#include "slcs.h"

namespace ArcSec {

static Arc::LogStream logcerr(std::cerr);

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Service_SLCS(cfg);
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
    ArcLib::Credential eec;
    eec.InquireRequest(x509_req, true);

    /* TODO: add extensions, e.g. saml extension parsed from SPService
    std::string ext_data("test extension data");
    if(!(eec.AddExtension("1.2.3.4", ext_data))) {
      std::cout<<"Failed to add an extension to certificate"<<std::endl;
    }
    */
    std::string dn("/O=KnowARC/OU=UIO/CN=Test001"); 
    //TODO: compose the base name from configuration and name from saml assertion?
 
    //Sign the certificate
    std::string x509_cert;
    ca_credential_->SignEECRequest(&eec, dn, x509_cert);

    //Compose response message
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
    Arc::XMLNode response = outpayload->NewChild("slcs:GetSLCSCertificateResponse");
    Arc::XMLNode x509_res_nd = response.NewChild("slcs:X509Response");
    x509_res_nd = x509_cert;

    outmsg.Payload(outpayload);
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
  ca_credential_ = new ArcLib::Credential(CAcert, CAkey, CAserial, 0, "", "");
}

Service_SLCS::~Service_SLCS(void) {
  if(ca_credential_) delete ca_credential_;
}

} // namespace ArcSec

service_descriptors ARC_SERVICE_LOADER = {
    { "slcs.service", 0, &ArcSec::get_service },
    { NULL, 0, NULL }
};

