#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>

#include <xmlsec/base64.h>
#include <xmlsec/errors.h>
#include <xmlsec/xmltree.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/xmlenc.h>
#include <xmlsec/templates.h>
#include <xmlsec/crypto.h>
#include <xmlsec/openssl/app.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>
#include <arc/GUID.h>
#include <arc/credential/Credential.h>

#include "../../hed/libs/common/XmlSecUtils.h"
#include "../../hed/libs/common/XMLSecNode.h"

#include "../../hed/libs/credential/saml_util.h"

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

typedef enum {
  HTTP_METHOD_NONE = -1,
  HTTP_METHOD_ANY,
  HTTP_METHOD_IDP_INITIATED,
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
  HTTP_METHOD_REDIRECT,
  HTTP_METHOD_SOAP,
  HTTP_METHOD_ARTIFACT_GET,
  HTTP_METHOD_ARTIFACT_POST
} HttpMethod;

///A example about how to compose a SAML <AttributeQuery> and call the Service_AA service, by
///using xmlsec library to compose <AttributeQuery> and process the <Response>.
int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "SAMLTest");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  // Create client chain
  logger.msg(Arc::INFO, "Creating client side chain");
  Arc::Config client_config("client.xml");
  if(!client_config) {
    logger.msg(Arc::ERROR, "Failed to load client configuration");
    return -1;
  };
  Arc::Loader client_loader(&client_config);
  logger.msg(Arc::INFO, "Client side MCCs are loaded");
  Arc::MCC* client_entry = client_loader["soap"];
  if(!client_entry) {
    logger.msg(Arc::ERROR, "Client chain does not have entry point");
    return -1;
  };

  //----------------------------------
  //     Compose AutheRequest
  //----------------------------------
  Arc::NS req_ns;
  req_ns["saml"] = SAML_NAMESPACE;
  req_ns["samlp"] = SAMLP_NAMESPACE;

  //Compose <samlp:AuthnRequest/>
  Arc::XMLNode authn_request(req_ns, "samlp:AuthnRequest");
  std::string sp_name1("https://sp.com/SAML"); //TODO
  std::string req_id = Arc::UUID();
  authn_request.NewAttribute("ID") = req_id;
  Arc::Time t1;
  std::string current_time1 = t1.str(Arc::UTCTime);
  authn_request.NewAttribute("IssueInstant") = current_time1;
  authn_request.NewAttribute("Version") = std::string("2.0");
  authn_request.NewChild("saml:Issuer") = sp_name1;
  
  Arc::XMLNode nameid_policy = authn_request.NewChild("samlp:NameIDPolicy");
  nameid_policy.NewAttribute("Format") = std::string("urn:oasis:names:tc:SAML:2.0:nameid-format:transient");
  nameid_policy.NewAttribute("SPNameQualifier") = sp_name1;
  //nameid_policy.NewAttribute("AllowCreate") = std::string("true"); 

  authn_request.NewAttribute("IsPassive") = std::string("false");
  //authn_request.NewAttribute() =  

  bool must_signed = true; //TODO: get the information from metadata
  
  HttpMethod httpmethod = HTTP_METHOD_REDIRECT; //TODO

  std::string privkey_file = "key.pem";
  std::string authnRequestUrl;
  if(httpmethod == HTTP_METHOD_REDIRECT) {
    std::string query = BuildDeflatedQuery(authn_request);
    authnRequestUrl = SignQuery(query, Arc::RSA_SHA1, privkey_file);
  }

  size_t f = authnRequestUrl.find("?");
  if(f == std::string::npos) return 0;
  std::string authnRequestQuery;
  authnRequestQuery = authnRequestUrl.substr(f+1);

 
  // -------------------------------------------------------
  //    Compose request and send to aa service
  // -------------------------------------------------------
  //Compose request
  Arc::NS ns;
  ns["saml"] = SAML_NAMESPACE;
  ns["samlp"] = SAMLP_NAMESPACE;

  std::string cert("./cert.pem");
  std::string key("./key.pem");
  std::string cafile("./ca.pem");
  ArcLib::Credential cred(cert, key, "", cafile);
  std::string local_dn = cred.GetDN();

  //Compose <samlp:AttributeQuery/>
  Arc::XMLNode attr_query(ns, "samlp:AttributeQuery");
  std::string sp_name("https://sp.com/SAML"); //TODO
  std::string query_id = Arc::UUID();
  attr_query.NewAttribute("ID") = query_id;
  Arc::Time t;
  std::string current_time = t.str(Arc::UTCTime);
  attr_query.NewAttribute("IssueInstant") = current_time;
  attr_query.NewAttribute("Version") = std::string("2.0");
  attr_query.NewChild("saml:Issuer") = sp_name;

  //<saml:Subject/>
  Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
  Arc::XMLNode name_id = subject.NewChild("saml:NameID");
  name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName");
  name_id = local_dn;

  //Add one or more <Attribute>s into AttributeQuery here, which means the Requestor would
  //get these <Attribute>s from AA
  //<saml:Attribute/>
  Arc::XMLNode attribute = attr_query.NewChild("saml:Attribute");
  attribute.NewAttribute("Name")=std::string("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
  attribute.NewAttribute("NameFormat")=std::string("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attribute.NewAttribute("FriendlyName")=std::string("eduPersonPrincipalName");

  Arc::init_xmlsec();
  Arc::XMLSecNode attr_query_secnd(attr_query);
  std::string attr_query_idname("ID");
  attr_query_secnd.AddSignatureTemplate(attr_query_idname, Arc::XMLSecNode::RSA_SHA1);
  if(attr_query_secnd.SignNode(key,cert)) {
    std::cout<<"Succeed to sign the signature under <samlp:AttributeQuery/>"<<std::endl;
  }

  std::string str;
  attr_query.GetXML(str);
  std::cout<<"++++ "<<str<<std::endl;

  Arc::NS soap_ns;
  Arc::SOAPEnvelope envelope(soap_ns);
  envelope.NewChild(attr_query);
  Arc::PayloadSOAP *payload = new Arc::PayloadSOAP(envelope);

  std::string tmp;
  payload->GetXML(tmp);
  std::cout<<"SOAP request from Arc client: ++++++++++++++++"<<tmp<<std::endl;
 
  // Send request
  Arc::MessageContext context;
  Arc::Message reqmsg;
  Arc::Message repmsg;
  Arc::MessageAttributes attributes_in;
  Arc::MessageAttributes attributes_out;
  reqmsg.Payload(payload);
  reqmsg.Attributes(&attributes_in);
  reqmsg.Context(&context);
  repmsg.Attributes(&attributes_out);
  repmsg.Context(&context);

  Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
  if(!status) {
    logger.msg(Arc::ERROR, "Request failed");
    return -1;
  };
  logger.msg(Arc::INFO, "Request succeed!!!");
  if(repmsg.Payload() == NULL) {
    logger.msg(Arc::ERROR, "There is no response");
    return -1;
  };

  Arc::PayloadSOAP* resp = NULL;
  try {
   resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
  } catch(std::exception&) { };
  if(resp == NULL) {
    logger.msg(Arc::ERROR, "Response is not SOAP");
    delete repmsg.Payload();  Arc::final_xmlsec();
    return -1;
  };

  // -------------------------------------------------------
  //   Comsume the response from aa service
  // -------------------------------------------------------
  //
  //Consume the response from AA
  Arc::XMLNode attr_resp;
  
  //<samlp:Response/>
  attr_resp = (*resp).Body().Child(0);
 
  //TODO: metadata processing.
  //std::string aa_name = attr_resp["saml:Issuer"]; 
 
  //Check validity of the signature on <samlp:Response/>
  std::string resp_idname = "ID";
  std::string cafile1 = "./ca.pem";
  std::string capath1 = "";
  Arc::XMLSecNode attr_resp_secnode(attr_resp);
  if(attr_resp_secnode.VerifyNode(resp_idname, cafile1, capath1)) {
    logger.msg(Arc::INFO, "Succeed to verify the signature under <samlp:Response/>");
  }
  else {
    logger.msg(Arc::ERROR, "Failed to verify the signature under <samlp:Response/>");
    delete repmsg.Payload(); Arc::final_xmlsec(); return -1;
  }
 
 
  //Check whether the "InResponseTo" is the same as the local ID
  std::string responseto_id = (std::string)(attr_resp.Attribute("InResponseTo"));
  if(query_id != responseto_id) {
    logger.msg(Arc::INFO, "The Response is not going to this end");
    delete repmsg.Payload(); Arc::final_xmlsec(); return -1;
  }

  std::string resp_time = attr_resp.Attribute("IssueInstant");

  //<samlp:Status/>
  std::string statuscode_value = attr_resp["samlp:Status"]["samlp:StatusCode"];
  if(statuscode_value == "urn:oasis:names:tc:SAML:2.0:status:Success")
    logger.msg(Arc::INFO, "The StatusCode is Success");

  //<saml:Assertion/>
  Arc::XMLNode assertion = attr_resp["saml:Assertion"];

  //TODO: metadata processing.
  //std::string aa_name = assertion["saml:Issuer"];
 
  //Check validity of the signature on <saml:Assertion/>
  std::string assertion_idname = "ID";
  std::string cafile2 = "./ca.pem";
  std::string capath2 = "";
  Arc::XMLSecNode assertion_secnode(assertion);
  if(assertion_secnode.VerifyNode(assertion_idname, cafile2, capath2)) {
    logger.msg(Arc::INFO, "Succeed to verify the signature under <saml:Assertion/>");
  }
  else {
    logger.msg(Arc::ERROR, "Failed to verify the signature under <saml:Assertion/>");
    delete repmsg.Payload(); Arc::final_xmlsec(); return -1;
  }

  //<saml:Subject/>, TODO: 
  Arc::XMLNode subject_nd = assertion["saml:Subject"];

  //<saml:Condition/>, TODO: Condition checking
  Arc::XMLNode cond_nd = assertion["saml:Conditions"];

  //<saml:AttributeStatement/>
  Arc::XMLNode attr_statement = assertion["saml:AttributeStatement"];

  //<saml:Attribute/>
  Arc::XMLNode attr_nd;
  std::vector<std::string> attributes_value;
  for(int i=0;;i++) {
    attr_nd = attr_statement["saml:Attribute"][i];
    if(!attr_nd) break;

    std::string name = attr_nd.Attribute("Name");
    std::string nameformat = attr_nd.Attribute("NameFormat");
    std::string friendname = attribute.Attribute("FriendlyName");

    Arc::XMLNode attr_value = attr_nd["saml:AttributeValue"];

    std::string str;
    str.append("Name=").append(friendname).append(" Value=").append(attr_value);
    attributes_value.push_back(str);
  }

  for(int i=0; i<attributes_value.size(); i++) {
    std::cout<<"Attribute Value: "<<attributes_value[i]<<std::endl;  
  }

  delete repmsg.Payload();

  Arc::final_xmlsec();

  return 0;
}
