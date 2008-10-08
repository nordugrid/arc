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
#include <arc/client/ClientInterface.h>
#include <arc/URL.h>

#include "../../hed/libs/common/XmlSecUtils.h"
#include "../../hed/libs/common/XMLSecNode.h"

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

///A example about how to compose a SAML <AttributeQuery> and call the voms saml service, by
///using xmlsec library to compose <AttributeQuery> and process the <Response>.
int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "SAMLTest");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  
  // -------------------------------------------------------
  //    Compose request and send to voms saml service
  // -------------------------------------------------------
  //Compose request
  Arc::NS ns;
  ns["saml"] = SAML_NAMESPACE;
  ns["samlp"] = SAMLP_NAMESPACE;

//  std::string cert("./usercert.pem");
//  std::string key("./userkey-nopass.pem");
//  std::string cafile("./1f0e8352.0");

  std::string cert("./cert.pem");
  std::string key("./key.pem");
  std::string cafile("./ca.pem");

  ArcLib::Credential cred(cert, key, "", cafile);
  //std::string local_dn = cred.GetDN();
  std::string local_dn("CN=test,O=UiO,ST=Oslo,C=NO");

  //Compose <samlp:AttributeQuery/>
  Arc::XMLNode attr_query(ns, "samlp:AttributeQuery");
  std::string query_id = Arc::UUID();
  attr_query.NewAttribute("ID") = query_id;
  Arc::Time t;
  std::string current_time = t.str(Arc::UTCTime);
  attr_query.NewAttribute("IssueInstant") = current_time;
  attr_query.NewAttribute("Version") = std::string("2.0");

  //<saml:Issuer/>
  std::string issuer_name = local_dn;
  Arc::XMLNode issuer = attr_query.NewChild("saml:Issuer");
  issuer = issuer_name;
  issuer.NewAttribute("Format") = std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName");

  //<saml:Subject/>
  Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
  Arc::XMLNode name_id = subject.NewChild("saml:NameID");
  name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName");
  name_id = local_dn;

  //Add one or more <Attribute>s into AttributeQuery here, which means the Requestor would
  //get these <Attribute>s from AA
  //<saml:Attribute/>
 /* 
  Arc::XMLNode attribute = attr_query.NewChild("saml:Attribute");
  attribute.NewAttribute("Name")=std::string("urn:oid:1.3.6.1.4.1.5923.1.1.1.6");
  attribute.NewAttribute("NameFormat")=std::string("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attribute.NewAttribute("FriendlyName")=std::string("eduPersonPrincipalName");
  */

  Arc::init_xmlsec();
/*Arc::XMLSecNode attr_query_secnd(attr_query);
  std::string attr_query_idname("ID");
  attr_query_secnd.AddSignatureTemplate(attr_query_idname, Arc::XMLSecNode::RSA_SHA1);
  if(attr_query_secnd.SignNode(key,cert)) {
    std::cout<<"Succeed to sign the signature under <samlp:AttributeQuery/>"<<std::endl;
  }
*/

  std::string str;
  attr_query.GetXML(str);
  std::cout<<"++++ "<<str<<std::endl;

  Arc::NS soap_ns;
  Arc::SOAPEnvelope envelope(soap_ns);
  envelope.NewChild(attr_query);
  Arc::PayloadSOAP request(envelope);

  std::string tmp;
  request.GetXML(tmp);
  std::cout<<"SOAP request from Arc client: ++++++++++++++++"<<tmp<<std::endl;
 
  // Send request
  Arc::MCCConfig cfg;
  if (!cert.empty())
    cfg.AddCertificate(cert);
  if (!key.empty())
    cfg.AddPrivateKey(key);
  if (!cafile.empty())
    cfg.AddCAFile(cafile);

  std::string path("/voms/saml/knowarc/services/AttributeAuthorityPortType");
  std::string service_url_str("https://squark.uio.no:8443");

//  std::string path("/voms/saml/testvo/services/AttributeAuthorityPortType");
//  std::string service_url_str("https://127.0.0.1:8443");

//  std::string path("/voms/saml/testvo/services/AttributeAuthorityPortType");
//  std::string service_url_str("http://127.0.0.1:8080");

//  std::string path("/voms/saml/omiieurope/services/AttributeAuthorityPortType");
//  std::string service_url_str("https://omii002.cnaf.infn.it:8443");

  Arc::URL service_url(service_url_str);
  Arc::ClientSOAP client(cfg, service_url.Host(), service_url.Port(), service_url.Protocol() == "https" ? 1:0, path);

  Arc::PayloadSOAP *response = NULL;
  Arc::MCC_Status status = client.process(&request,&response);
  if (!response) {
    logger.msg(Arc::ERROR, "Request failed: No response");
    return -1;
  }
  if (!status) {
    std::string tmp;
    response->GetXML(tmp);
    std::cout<<"Response: "<<tmp<<std::endl;
    logger.msg(Arc::ERROR, "Request failed: Error");
    return -1;
  }

  response->GetXML(str);
  std::cout<<"Response: "<<str<<std::endl;

  // -------------------------------------------------------
  //   Comsume the response from saml voms service
  // -------------------------------------------------------
  //Consume the response from AA
  Arc::XMLNode attr_resp;
  
  //<samlp:Response/>
  attr_resp = (*response).Body().Child(0);
 
  //TODO: metadata processing.
  //std::string aa_name = attr_resp["saml:Issuer"]; 
 
  //Check validity of the signature on <samlp:Response/>
/*
  std::string resp_idname = "ID";
  std::string cafile1 = "./ca.pem";
  std::string capath1 = "";
  Arc::XMLSecNode attr_resp_secnode(attr_resp);
  if(attr_resp_secnode.VerifyNode(resp_idname, cafile1, capath1)) {
    logger.msg(Arc::INFO, "Succeed to verify the signature under <samlp:Response/>");
  }
  else {
    logger.msg(Arc::ERROR, "Failed to verify the signature under <samlp:Response/>");
    Arc::final_xmlsec(); return -1;
  }
*/
 
  //Check whether the "InResponseTo" is the same as the local ID
  std::string responseto_id = (std::string)(attr_resp.Attribute("InResponseTo"));
  if(query_id != responseto_id) {
    logger.msg(Arc::INFO, "The Response is not going to this end");
    Arc::final_xmlsec(); return -1;
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
/*
  std::string assertion_idname = "ID";
  std::string cafile2 = "./ca.pem";
  std::string capath2 = "";
  Arc::XMLSecNode assertion_secnode(assertion);
  if(assertion_secnode.VerifyNode(assertion_idname, cafile2, capath2)) {
    logger.msg(Arc::INFO, "Succeed to verify the signature under <saml:Assertion/>");
  }
  else {
    logger.msg(Arc::ERROR, "Failed to verify the signature under <saml:Assertion/>");
    Arc::final_xmlsec(); return -1;
  }
*/

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

    for(int j=0;;j++) {
      Arc::XMLNode attr_value = attr_nd["saml:AttributeValue"][j];
      if(!attr_value) break;
      std::string str;
      str.append("Name=").append(name).append(" Value=").append(attr_value);
      attributes_value.push_back(str);
    }
  }

  for(int i=0; i<attributes_value.size(); i++) {
    std::cout<<"Attribute Value: "<<attributes_value[i]<<std::endl;  
  }

  Arc::final_xmlsec();
  return 0;
}
