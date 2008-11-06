#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <arc/DateTime.h>
#include <arc/GUID.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/PayloadRaw.h>
#include <arc/security/SecHandler.h>
#include <arc/credential/Credential.h>
#include <arc/client/ClientInterface.h>

#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include <arc/xmlsec/saml_util.h>

#include "SPService.h"

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new SPService::Service_SP(cfg);
}

service_descriptors ARC_SERVICE_LOADER = {
    { "saml.sp", 0, &get_service },
    { NULL, 0, NULL }
};

namespace SPService {

Service_SP::Service_SP(Arc::Config *cfg):Service(cfg),logger(Arc::Logger::rootLogger, "SAML2SP") {
  std::string metadata_file = "test_metadata.xml";
  metadata_node_.ReadFromFile(metadata_file);
  std::string str;
  metadata_node_.GetXML(str);
  //std::cout<<"Metadata: "<<str<<std::endl;
}

Service_SP::~Service_SP(void) {
}

Arc::MCC_Status Service_SP::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  // Check authentication and authorization
  if(!ProcessSecHandlers(inmsg, "incoming")) {
    logger.msg(Arc::ERROR, "saml2SP: Unauthorized");
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
  };
  // Both input and output are supposed to be HTTP 
  // Extracting payload
  Arc::PayloadRawInterface* inpayload = (Arc::PayloadRawInterface*)(inmsg.Payload());
  if(!inpayload) {
    logger.msg(Arc::WARNING, "empty input payload");
  };

  //Analyzing http request from user agent
  //for(Arc::AttributeIterator i = inmsg.Attributes()->getAll();i.hasMore();++i) {
  //  std::cout<<"Attribute: "<<i.key()<<"=="<<*i<<std::endl;
  //}
  //Get the IdP name from the request
  //Here we require the user agent to provide the idp name instead of the 
  //WRYF(where are you from) or Discovery Service in some other implementation
  //like Shibboleth
  std::string idp_name(inpayload->Content());

  //Compose <samlp:AuthnRequest/>
  Arc::NS saml_ns;
  saml_ns["saml"] = SAML_NAMESPACE;
  saml_ns["samlp"] = SAMLP_NAMESPACE;
  Arc::XMLNode authn_request(saml_ns, "samlp:AuthnRequest");
  std::string sp_name("https://squark.uio.no/shibboleth-sp"); //TODO
  std::string req_id = Arc::UUID();
  authn_request.NewAttribute("ID") = req_id;
  Arc::Time t1;
  std::string current_time1 = t1.str(Arc::UTCTime);
  authn_request.NewAttribute("IssueInstant") = current_time1;
  authn_request.NewAttribute("Version") = std::string("2.0");

  //Get url of assertion consumer service from metadata
  std::string assertion_consumer_url;
  for(int i = 0;;i++) {
    Arc::XMLNode nd = metadata_node_.Child(i);
    if(!nd) break;
    if(sp_name == (std::string)(nd.Attribute("entityID"))) {
      for(int j = 0;; j++) {
        Arc::XMLNode sp_nd = nd.Child(j);
        if(!sp_nd) break;
        if(MatchXMLName(sp_nd,"SPSSODescriptor")) {
          for(int k = 0;;k++) {
            Arc::XMLNode assertionconsumer_nd = sp_nd.Child(k);
            if(!assertionconsumer_nd) break;        
            if(MatchXMLName(assertionconsumer_nd, "AssertionConsumerService")) {
              if((std::string)(assertionconsumer_nd.Attribute("Binding")) == "urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST")
                assertion_consumer_url = (std::string)(assertionconsumer_nd.Attribute("Location"));
            }
            if(!assertion_consumer_url.empty()) break;
          }
        }
        if(!assertion_consumer_url.empty()) break;
      }
    }
  }
  authn_request.NewAttribute("AssertionConsumerServiceURL") = assertion_consumer_url;

  //Get url of sso service from metadata
  std::string sso_url;
  for(int i = 0;;i++) {
    Arc::XMLNode nd = metadata_node_.Child(i);
    if(!nd) break;
    if(idp_name == (std::string)(nd.Attribute("entityID"))) {
      for(int j = 0;; j++) {
        Arc::XMLNode idp_nd = nd.Child(j);
        if(!idp_nd) break;
        if(MatchXMLName(idp_nd,"IDPSSODescriptor")) {
          for(int k = 0;;k++) {
            Arc::XMLNode sso_nd = idp_nd.Child(k);
            if(!sso_nd) break;
            if(MatchXMLName(sso_nd, "SingleSignOnService")) {
              if((std::string)(sso_nd.Attribute("Binding")) == "urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect")
                sso_url = (std::string)(sso_nd.Attribute("Location"));
            }
            if(!sso_url.empty()) break;
          }
        }
        if(!sso_url.empty()) break;
      }
    }
  }
  authn_request.NewAttribute("Destination") = sso_url;
  authn_request.NewAttribute("ProtocolBinding") = std::string("urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST");
  authn_request.NewChild("saml:Issuer") = sp_name;

  Arc::XMLNode nameid_policy = authn_request.NewChild("samlp:NameIDPolicy");
  nameid_policy.NewAttribute("AllowCreate") = std::string("1");

  bool must_signed = false; //TODO: get the information from metadata

  std::string cert_file = "testcert.pem";  //TODO: another configuration file?
  std::string privkey_file = "testkey-nopass.pem";

  std::string authnRequestQuery;
  std::string query = BuildDeflatedQuery(authn_request);
  //std::cout<<"AuthnRequest after deflation: "<<query<<std::endl;
  if(must_signed) {
    authnRequestQuery = SignQuery(query, Arc::RSA_SHA1, privkey_file);
    //std::cout<<"After signature: "<<authnRequestQuery<<std::endl;
  }
  else authnRequestQuery = query;

  std::string authnRequestUrl;
  authnRequestUrl = sso_url + "?SAMLRequest=" + authnRequestQuery;

  //Return the composed url back to user agent through http
  Arc::PayloadRaw* outpayload = NULL;
  outpayload = new Arc::PayloadRaw;
  outpayload->Insert(authnRequestUrl.c_str(),0, authnRequestUrl.size());
  outmsg.Attributes()->set("HTTP:CODE","302");
  outmsg.Attributes()->set("HTTP:REASON","Moved Temporarily");
  std::string outcontent(outpayload->Content());
  delete outmsg.Payload(outpayload);

  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace SPService

