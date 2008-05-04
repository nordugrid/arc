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

#include <lasso/lasso.h>
#include <lasso/saml-2.0/assertion_query.h>
#include <lasso/xml/saml-2.0/saml2_attribute.h>
#include <lasso/xml/saml-2.0/saml2_attribute_value.h>
#include <lasso/xml/saml-2.0/samlp2_attribute_query.h>
#include <lasso/xml/saml-2.0/saml2_assertion.h>
#include <lasso/xml/saml-2.0/saml2_audience_restriction.h>
#include <lasso/xml/saml-2.0/samlp2_response.h>
#include <lasso/xml/saml-2.0/saml2_attribute_statement.h>
#include <lasso/xml/xml.h>

#include "aaservice.h"

namespace ArcSec {

static Arc::LogStream logcerr(std::cerr);

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Service_AA(cfg);
}

Arc::MCC_Status Service_AA::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Service_AA::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  std::string identity_aa("<Identity xmlns=\"http://www.entrouvert.org/namespaces/lasso/0.0\" Version=\"2\">\
    <Federation xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\"\
       RemoteProviderID=\"https://sp.com/SAML\"\
       FederationDumpVersion=\"2\">\
      <RemoteNameIdentifier>\
        <saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName\"></saml:NameID>\
      </RemoteNameIdentifier>\
    </Federation>\
   </Identity>");
  XMLNode id_node(identity_aa);
  //Set the <saml:NameID> by using the DN of peer certificate
  std::string peer_dn = inmsg.Attributes()->get("TLS:PEERDN");
  id_node["Federation"]["RemoteNameIdentifier"]["saml:NameID"] = peer_dn;
  id_node.GetXML(identity_aa);

  lasso_profile_set_identity_from_dump(LASSO_PROFILE(assertion_query_), identity_aa.c_str());

  // Extracting payload
  Arc::PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) {
    logger.msg(Arc::ERROR, "input is not SOAP");
    return make_soap_fault(outmsg);
  }

  std::string assertionRequestBody;
  inpayload.GetXML(assertionRequestBody);
  int rc = lasso_assertion_query_process_request_msg(assertion_query_, assertionRequestBody);
  if(rc != 0) { logger.msg(Arc::ERROR, "lasso_assertion_query_process_request_msg failed");

  rc = lasso_assertion_query_validate_request(assertionqry1);
  

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

Service_AA::Service_AA(Arc::Config *cfg):Service(cfg), logger_(Arc::Logger::rootLogger, "AA_Service"), eval(NULL) {
  logger_.addDestination(logcerr);
  assertion_query_ = NULL;
  lasso_init();
  LassoServer *server;
  server = lasso_server_new("./saml_metadata.xml", "./private-key-raw.pem", NULL, "./certificate.pem"); 
  assertion_query_ = lasso_assertion_query_new (server);
  if(assertion_query_ == NULL) logger.msg(Arc::DEBUG, "lasso_assertion_query_new() failed"); 
}

Service_AA::~Service_AA(void) {
  if(eval)
    delete eval;
  eval = NULL;
}

} // namespace ArcSec

service_descriptors ARC_SERVICE_LOADER = {
    { "aa.service", 0, &ArcSec::get_service },
    { NULL, 0, NULL }
};

