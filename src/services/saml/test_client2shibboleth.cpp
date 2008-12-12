#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>

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

#include "../../hed/libs/xmlsec/XmlSecUtils.h"
#include "../../hed/libs/xmlsec/XMLSecNode.h"
#include "../../hed/libs/xmlsec/saml_util.h"

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

///A example about how to mimic the process of Shibboleth SAML2SSO and SAML2AttributeQuery profile.
///The intention is to use the Shibboleth IdP for authentication and attribute query in order to 
///demostrate the saml2 standard compatibility and utilize the Shib IdP to get the authentication 
///result and attributes for protecting the web services.
/**Some idea about how to integrate shibboleth functionality into HED.
 * The use case about intergrating Shib IdP is the same as the use case in Shibboleth (SAML2SSO and 
 * SAML2AttributeQuery), except that we are protecting normal web service instead of web 
 * application in Shib SP, and we are using some specific code in the client to 
 * interact with service instead of the Web browser in Shibboleth client.
 * 1. Client access service. A security handler in service will interact with the security handler in 
 * client to do something like: Where are you from? Composing AuthnRequest, and then return the AuthnRequest 
 * to client. 
 * 2. Client authencate with Shib IdP by using whatever mechanism. 
 * 3. Client get the AuthnRequest and POST it to Shib IdP by http protocol, and get back b64 encoded Saml 
 * response which included the encrypted Authentication assertion from Shib IdP. 
 * 4. Client (the above security handler in client side) POST the above saml response to service; then 
 * the above security handler in service side will decode, verify this saml response, and decrypt the 
 * saml assertion and saml NameID in this saml assertion. 
 * 5.The security handler in service side use this NameID to compose a saml Attribute Query, and send it 
 * directly to Shib IdP, and finally get back the attributes which will be used for authorization. 
 *
 * So the basic idea is to mimic the client-agent and Shib SP in the typical Shibboleth scenario, but 
 * keep the Shib IdP. The benifit it that we can utilize the widely-deployed Shib IdP for protecting 
 * web/grid service.
 */
int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "SAMLTest");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  Arc::NS saml_ns;
  saml_ns["saml"] = SAML_NAMESPACE;
  saml_ns["samlp"] = SAMLP_NAMESPACE;

  //----------------------------------
  //Client-Agent and SP: Compose AuthnRequest
  //Client-Agent access SP, SP generate AuthnRequest and respond it to Client-Agent
  //----------------------------------

  //Compose <samlp:AuthnRequest/>
  Arc::XMLNode authn_request(saml_ns, "samlp:AuthnRequest");
  //std::string sp_name("https://sp.testshib.org/shibboleth-sp"); //TODO
  std::string sp_name("https://squark.uio.no/shibboleth-sp");
  std::string req_id = Arc::UUID();
  authn_request.NewAttribute("ID") = req_id;
  Arc::Time t1;
  std::string current_time1 = t1.str(Arc::UTCTime);
  authn_request.NewAttribute("IssueInstant") = current_time1;
  authn_request.NewAttribute("Version") = std::string("2.0");
  //authn_request.NewAttribute("AssertionConsumerServiceURL") = std::string("https://sp.testshib.org/Shibboleth.sso/SAML2/POST");
  authn_request.NewAttribute("AssertionConsumerServiceURL") = std::string("https://squark.uio.no/Shibboleth.sso/SAML2/POST");


  std::string destination("https://squark.uio.no:8443/idp/profile/SAML2/Redirect/SSO");
  //std::string destination("https://idp.testshib.org/idp/profile/SAML2/Redirect/SSO");

  authn_request.NewAttribute("Destination") = destination;
  authn_request.NewAttribute("ProtocolBinding") = std::string("urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST");
   
  authn_request.NewChild("saml:Issuer") = sp_name;
  
  Arc::XMLNode nameid_policy = authn_request.NewChild("samlp:NameIDPolicy");
  nameid_policy.NewAttribute("AllowCreate") = std::string("1");

  //nameid_policy.NewAttribute("Format") = std::string("urn:oasis:names:tc:SAML:2.0:nameid-format:transient");
  //nameid_policy.NewAttribute("SPNameQualifier") = sp_name;

  //authn_request.NewAttribute("IsPassive") = std::string("false");
  //authn_request.NewAttribute() =  

  bool must_signed = false; //TODO: get the information from metadata
  
  HttpMethod httpmethod = HTTP_METHOD_REDIRECT; //TODO

  std::string str1;
  authn_request.GetXML(str1);
  std::cout<<"AuthnRequest:  "<<str1<<std::endl;

  std::string cert_file;// = "testcert.pem";
  std::string privkey_file = "testkey-nopass.pem";
  //TestShib use the certificate for SAML2 SSO
  std::string ca_file = "cacert.pem"; // = "cacert_testshib.pem";

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
  // Client-Agent: Send the AuthnRequest to IdP, 
  // (IP-based authentication, and Username/Password authentication) 
  // get back b64 encoded saml response, and 
  // send this saml response to SP
  // -------------------------------------------
  //
  //

  Arc::MCCConfig cfg;
  //if (!cert_file.empty())
  //  cfg.AddCertificate(cert_file);
  //if (!privkey_file.empty())
  //  cfg.AddPrivateKey(privkey_file);
  if (!ca_file.empty())
    cfg.AddCAFile(ca_file);

  Arc::URL url(authnRequestUrl);
  Arc::ClientHTTP client(cfg, url);
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

  //The following code is for authentication (username/password)
  std::string resp_html;
  Arc::HTTPClientInfo redirect_info = info;
  do {
    std::cout<<"Code: "<<redirect_info.code<<"Reason: "<<redirect_info.reason<<"Size: "<<
    redirect_info.size<<"Type: "<<redirect_info.type<<"Set-Cookie: "<<redirect_info.cookie<<
    "Location: "<<redirect_info.location<<std::endl;
    if(redirect_info.code != 302) break;
    
    Arc::URL redirect_url(redirect_info.location);
    Arc::ClientHTTP redirect_client(cfg, redirect_url);

    Arc::PayloadRaw redirect_request;
    Arc::PayloadRawInterface *redirect_response = NULL;

    std::map<std::string, std::string> http_attributes;
    if(!(redirect_info.cookie.empty())) http_attributes["Cookie"]=redirect_info.cookie;

    Arc::MCC_Status redirect_status = redirect_client.process("GET", http_attributes, 
                              &redirect_request, &redirect_info, &redirect_response);
    if (!redirect_response) {
      logger.msg(Arc::ERROR, "Request failed: No response");
      return -1;
    }
    if (!redirect_status) {
      logger.msg(Arc::ERROR, "Request failed: Error");
      return -1;
    }
    char* content = redirect_response->Content();
    if(content!=NULL) {
      resp_html.assign(redirect_response->Content());
      size_t pos = resp_html.find("j_username"); 
      if(pos!=std::string::npos) break;
    }
  } while(1);

  //Arc::URL redirect_url_final("https://idp.testshib.org:443/idp/Authn/UserPassword");
  Arc::URL redirect_url_final(info.location);
  Arc::ClientHTTP redirect_client_final(cfg, redirect_url_final);
  Arc::PayloadRaw redirect_request_final;
  //std::string login_html("j_username=myself&j_password=myself");
  //std::string login_html("j_username=root&j_password=aa1122");
  std::string login_html("j_username=staff&j_password=123456");
  redirect_request_final.Insert(login_html.c_str(),0,login_html.size());
  std::map<std::string, std::string> http_attributes;
  http_attributes["Content-Type"] = "application/x-www-form-urlencoded";
  //Use the first cookie
  if(!(info.cookie.empty()))            
    http_attributes["Cookie"]=info.cookie; 
  Arc::PayloadRawInterface *redirect_response_final = NULL;
  Arc::HTTPClientInfo redirect_info_final;
  Arc::MCC_Status redirect_status_final = redirect_client_final.process("POST", http_attributes, 
        &redirect_request_final, &redirect_info_final, &redirect_response_final);
  if (!redirect_response_final) {
    logger.msg(Arc::ERROR, "Request failed: No response");
    return -1;
  }
  if (!redirect_status_final) {
    logger.msg(Arc::ERROR, "Request failed: Error");
    return -1;
  }

  std::cout<<"http code: "<<redirect_info_final.code<<std::endl;

  std::string html_body;
  for(int i = 0;;i++) {
    char* buf = redirect_response_final->Buffer(i);
    if(buf == NULL) break;
    html_body.append(redirect_response_final->Buffer(i), redirect_response_final->BufferSize(i));
    std::cout<<"Buffer: "<<buf<<std::endl;
  }

  std::string type = redirect_info_final.type;
  size_t pos1 = type.find(';');
  if (pos1 != std::string::npos)
    type = type.substr(0, pos1);
  if(type != "text/html")
    std::cerr<<"The html response from IdP is with wrong format"<<std::endl;

  //The authentication is finished and the html response with saml response is returned
 
  //--------------------------------------
  //SP: Get the b64 encoded saml response from Client-Agent, and decode/verify/decrypt it
  //-------------------------------------
  Arc::XMLNode html_node(html_body);
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
  Arc::XMLSecNode sec_samlresp_nd(node);
  //Since the certificate from idp.testshib.org which signs the saml response is self-signed 
  //certificate, only check the signature here.
  if(sec_samlresp_nd.VerifyNode(idname,"", "", false)) {
    logger.msg(Arc::INFO, "Succeeded to verify the signature under <samlp:Response/>");
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

#if 0
  //-----------------------
  //SP: AttributeQuery
  //-----------------------
  //Compose <samlp:AttributeQuery/>
  std::string cert = "testcert.pem";
  std::string key = "testkey-nopass.pem";
  std::string cafile = "cacert.pem";

  ArcLib::Credential cred(cert, key, "", cafile);
  std::string local_dn = cred.GetDN();

  Arc::XMLNode attr_query(saml_ns, "samlp:AttributeQuery");
  std::string query_id = Arc::UUID();
  attr_query.NewAttribute("ID") = query_id;
  Arc::Time t;
  std::string current_time = t.str(Arc::UTCTime);
  attr_query.NewAttribute("IssueInstant") = current_time;
  attr_query.NewAttribute("Version") = std::string("2.0");

  Arc::XMLNode issuer = attr_query.NewChild("saml:Issuer");
  issuer = sp_name;

  //<saml:Subject/>
  Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
  Arc::XMLNode name_id = subject.NewChild("saml:NameID");
  name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:2.0:nameid-format:transient");
  name_id = (std::string)decrypted_nameid_nd;

  //Add one or more <Attribute>s into AttributeQuery
/*
  Arc::XMLNode attribute = attr_query.NewChild("saml:Attribute");
  attribute.NewAttribute("Name")=std::string("urn:oid:1.3.6.1.4.1.5923.1.1.1.1");
  attribute.NewAttribute("NameFormat")=std::string("urn:oasis:names:tc:SAML:2.0:attrname-format:uri");
  attribute.NewAttribute("FriendlyName")=std::string("eduPersonAffiliation");
*/

  std::string str;
  attr_query.GetXML(str);
  std::cout<<"Attribute Query: "<<str<<std::endl;

  Arc::NS soap_ns;
  Arc::SOAPEnvelope envelope(soap_ns);
  envelope.NewChild(attr_query);
  Arc::PayloadSOAP attrqry_request(envelope);
 
  // Send request
std::cout<<"---------------------------------------------------------------------------"<<std::endl;
  Arc::MCCConfig attrqry_cfg;
  if (!cert_file.empty())
    attrqry_cfg.AddCertificate(cert_file);
  if (!privkey_file.empty())
    attrqry_cfg.AddPrivateKey(privkey_file);
  //TestShib use another certificate for AttributeQuery
  std::string ca_file1 = "cacert_testshib_idp.pem"; 
  if (!ca_file1.empty())
    attrqry_cfg.AddCAFile(ca_file1);

  std::string attrqry_path("/idp/profile/SAML2/SOAP/AttributeQuery");
  //std::string service_url_str("https://squark.uio.no:8443");
  std::string service_url_str("https://idp.testshib.org:8443");

  Arc::URL service_url(service_url_str + attrqry_path);
  Arc::ClientSOAP attrqry_client(attrqry_cfg, service_url);

  Arc::PayloadSOAP *attrqry_response = NULL;
  Arc::MCC_Status attrqry_status = attrqry_client.process(&attrqry_request,&attrqry_response);
  if (!attrqry_response) {
    logger.msg(Arc::ERROR, "Request failed: No response");
    return -1;
  }
  if (!attrqry_status) {
    std::string tmp;
    attrqry_response->GetXML(tmp);
    std::cout<<"Response: "<<tmp<<std::endl;
    logger.msg(Arc::ERROR, "Request failed: Error");
    return -1;
  }
  attrqry_response->GetXML(str);
  std::cout<<"Response for Attribute Query: "<<str<<std::endl;
std::cout<<"---------------------------------------------------------------------------"<<std::endl;


  // -------------------------------------------------------
  //   Comsume the saml response
  // -------------------------------------------------------
  //Consume the response from AA
  Arc::XMLNode saml_response;
  saml_response = (*attrqry_response).Body().Child(0);

  std::string resp_time = saml_response.Attribute("IssueInstant");

  //<samlp:Status/>
  std::string statuscode_value = saml_response["samlp:Status"]["samlp:StatusCode"];
  if(statuscode_value == "urn:oasis:names:tc:SAML:2.0:status:Success")
    logger.msg(Arc::INFO, "The StatusCode is Success");
  
  //<saml:Assertion/>
  Arc::XMLNode assertion = saml_response["saml:Assertion"];

  //We don't need to verify the saml signature, because the idp.testshib.org does not return 
  //saml response with signature
 
  //<saml:Condition/>, TODO: Condition checking
  Arc::XMLNode cond_nd = assertion["saml:Conditions"];

  //<saml:AttributeStatement/>
  Arc::XMLNode attr_statement = assertion["saml:AttributeStatement"];

  //<saml:Attribue/>
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

  delete attrqry_response;

#endif

  delete response;
  Arc::final_xmlsec();
  return 0;
}
