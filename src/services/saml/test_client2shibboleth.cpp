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
#include <arc/URL.h>
#include <arc/credential/Credential.h>

#include <arc/message/MCC.h>
#include <arc/message/PayloadRaw.h>
#include <arc/client/ClientInterface.h>

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

  //----------------------------------
  //Client-Agent and SP: Compose AuthnRequest
  //Client-Agent access SP, SP generate AuthnRequest and respond it to Client-Agent
  //----------------------------------
  Arc::NS req_ns;
  req_ns["saml"] = SAML_NAMESPACE;
  req_ns["samlp"] = SAMLP_NAMESPACE;

  //Compose <samlp:AuthnRequest/>
  Arc::XMLNode authn_request(req_ns, "samlp:AuthnRequest");
  //std::string sp_name1("https://sp.testshib.org/shibboleth-sp"); //TODO
  std::string sp_name1("https://squark.uio.no/shibboleth-sp");
  std::string req_id = Arc::UUID();
  authn_request.NewAttribute("ID") = req_id;
  Arc::Time t1;
  std::string current_time1 = t1.str(Arc::UTCTime);
  authn_request.NewAttribute("IssueInstant") = current_time1;
  authn_request.NewAttribute("Version") = std::string("2.0");
  //authn_request.NewAttribute("AssertionConsumerServiceURL") = std::string("https://sp.testshib.org/Shibboleth.sso/SAML2/POST");
  authn_request.NewAttribute("AssertionConsumerServiceURL") = std::string("https://squark.uio.no/Shibboleth.sso/SAML2/POST");


  std::string destination("https://squark.uio.no:8443/idp/profile/SAML2/Redirect/SSO");
  authn_request.NewAttribute("Destination") = destination;
  authn_request.NewAttribute("ProtocolBinding") = std::string("urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST");
   
  authn_request.NewChild("saml:Issuer") = sp_name1;
  
  Arc::XMLNode nameid_policy = authn_request.NewChild("samlp:NameIDPolicy");
  nameid_policy.NewAttribute("AllowCreate") = std::string("1");

  //nameid_policy.NewAttribute("Format") = std::string("urn:oasis:names:tc:SAML:2.0:nameid-format:transient");
  //nameid_policy.NewAttribute("SPNameQualifier") = sp_name1;

  //authn_request.NewAttribute("IsPassive") = std::string("false");
  //authn_request.NewAttribute() =  

  bool must_signed = false; //TODO: get the information from metadata
  
  HttpMethod httpmethod = HTTP_METHOD_REDIRECT; //TODO

  std::string str1;
  authn_request.GetXML(str1);
  std::cout<<"AuthnRequest:  "<<str1<<std::endl;

  std::string cert_file = "testcert.pem";
  std::string privkey_file = "testkey-nopass.pem";
  std::string ca_file = "cacert.pem";

  std::string authnRequestQuery;
  if(httpmethod == HTTP_METHOD_REDIRECT) {
    std::string query = BuildDeflatedQuery(authn_request);
    std::cout<<"AuthnRequest after deflation: "<<query<<std::endl;
    if(must_signed) {
      authnRequestQuery = SignQuery(query, Arc::RSA_SHA1, privkey_file);
      std::cout<<"After signature: "<<authnRequestQuery<<std::endl;
    }
    else authnRequestQuery = query;
  }
  
  std::string authnRequestUrl;
  authnRequestUrl = destination + "?SAMLRequest=" + authnRequestQuery;
  std::cout<<"autnRequestUrl: "<<authnRequestUrl<<std::endl;

  // -------------------------------------------
  // Client-Agent: Send the AuthnRequest to IdP, get back b64 encoded saml response, and 
  // send this saml response to SP
  // -------------------------------------------
  Arc::MCCConfig cfg;
  if (!cert_file.empty())
    cfg.AddCertificate(cert_file);
  if (!privkey_file.empty())
    cfg.AddPrivateKey(privkey_file);
  if (!ca_file.empty())
    cfg.AddCAFile(ca_file);

  Arc::URL url(authnRequestUrl);
  std::string url_str = url.str();
  size_t pos = url_str.find(":");
  pos = url_str.find(":", pos+1);
  pos = url_str.find("/", pos+1);
  std::string path = url_str.substr(pos);

  std::cout<<path<<std::endl;

  Arc::ClientHTTP client(cfg, url.Host(), url.Port(), url.Protocol() == "https" ? 1:0, path);

  std::cout<<"URL: "<<url.str()<<std::endl<<"Host: "<<url.Host()<<std::endl<<"Port: "<<url.Port()<<std::endl<<"Protocol: "<<url.Protocol()<<std::endl;

  Arc::PayloadRaw request;
  Arc::PayloadRawInterface *response = NULL;
  Arc::HTTPClientInfo info;
  Arc::MCC_Status status = client.process("GET", &request, &info, &response);
  if (!response) {
    logger.msg(Arc::ERROR, "Request failed: No response");
    return -1;
  }
  if (!status) {
    logger.msg(Arc::ERROR, "Request failed: Error");
    return -1;
  }

  std::cout<<"Response content: "<<response->Content()<<std::endl;

  std::string type = info.type;
  size_t pos1 = type.find(';');
  if (pos1 != std::string::npos)
    type = type.substr(0, pos1);

  if(type != "text/html")
    std::cerr<<"The html from IdP is with wrong format"<<std::endl;
 
  //--------------------------------------
  //SP: Get the b64 encoded saml response from Client-Agent, and decode/verify/decrypt it
  //-------------------------------------
  Arc::XMLNode html_node(response->Content());
  std::string saml_resp = html_node["body"]["form"]["div"]["input"].Attribute("value");
  std::cout<<"SAML Response: "<<saml_resp<<std::endl;

  Arc::XMLNode node;

  //Decode the saml response (b64 format)
  Arc::BuildNodefromMsg(saml_resp, node);  
  std::string decoded_saml_resp;
  node.GetXML(decoded_saml_resp);
  std::cout<<"Decoded SAML Response: "<<decoded_saml_resp<<std::endl;

  Arc::init_xmlsec();

  //Verify the signature of saml response
  std::string idname = "ID";
  std::string ca_path = "";
  Arc::XMLSecNode sec_samlresp_nd(node);
  if(sec_samlresp_nd.VerifyNode(idname, ca_file, ca_path)) {
    logger.msg(Arc::INFO, "Succeed to verify the signature under <samlp:Response/>");
  }
  else {
    logger.msg(Arc::ERROR, "Failed to verify the signature under <samlp:Response/>");
  }
  
  //Decrypte the encrypted saml assertion in this saml response
  Arc::XMLNode assertion_nd = node["saml:EncryptedAssertion"];
  std::string saml_assertion;
  assertion_nd.GetXML(saml_assertion);
  std::cout<<"Encrypted saml assertion: "<<saml_assertion<<std::endl;

  Arc::XMLSecNode sec_assertion_nd(assertion_nd);
  Arc::XMLNode decrypted_assertion_nd; 
  bool r = sec_assertion_nd.DecryptNode(privkey_file, decrypted_assertion_nd);
  if(!r) { std::cout<<"Can not decrypted the EncryptedAssertion from saml response"<<std::endl; return 0; }

  std::string decrypted_saml_assertion;
  decrypted_assertion_nd.GetXML(decrypted_saml_assertion);
  std::cout<<"Decrypted SAML Assertion: "<<decrypted_saml_assertion<<std::endl;

  //Decrypted the <saml:EncryptedID> in the above saml assertion
  Arc::XMLNode nameid_nd = decrypted_assertion_nd["saml:Subject"]["saml:EncryptedID"];
  std::string nameid;
  nameid_nd.GetXML(nameid);
  std::cout<<"Encrypted name id: "<<nameid<<std::endl;

  Arc::XMLSecNode sec_nameid_nd(nameid_nd);
  Arc::XMLNode decrypted_nameid_nd;
  r = sec_nameid_nd.DecryptNode(privkey_file, decrypted_nameid_nd);
  if(!r) { std::cout<<"Can not decrypted the EncryptedID from saml assertion"<<std::endl; return 0; }

  std::string decrypted_nameid;
  decrypted_nameid_nd.GetXML(decrypted_nameid);
  std::cout<<"Decrypted SAML NameID: "<<decrypted_nameid<<std::endl;


  //-----------------------
  //SP: AttributeQuery
  //-----------------------

  delete response;
  Arc::final_xmlsec();
  return 0;
}
