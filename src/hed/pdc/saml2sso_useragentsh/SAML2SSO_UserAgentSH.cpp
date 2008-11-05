#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/loader/SecHandlerLoader.h>
#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/client/ClientInterface.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include <arc/xmlsec/saml_util.h>

#include "SAML2SSO_UserAgentSH.h"

static Arc::Logger logger(Arc::Logger::rootLogger, "SAML2SSO_UserAgentSH");

ArcSec::SecHandler* ArcSec::SAML2SSO_UserAgentSH::get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new ArcSec::SAML2SSO_UserAgentSH(cfg,ctx);
}

/*
sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "saml2ssouseragent.handler", 0, &get_sechandler},
    { NULL, 0, NULL }
};
*/

namespace ArcSec {
using namespace Arc;

SAML2SSO_UserAgentSH::SAML2SSO_UserAgentSH(Config *cfg,ChainContext*):SecHandler(cfg){
  if(!init_xmlsec()) return;
}

SAML2SSO_UserAgentSH::~SAML2SSO_UserAgentSH() {
  final_xmlsec();
}

bool SAML2SSO_UserAgentSH::Handle(Arc::Message* msg){

  // -------------------------------------------
  // User-Agent: Send an empty http request to SP, the destination url 
  // is got from the url of the main chain (but using 8443 as the port 
  // instead of the port used in main chain); here we suppose together 
  // with the service to which this client tries to access, there is 
  // a http service (on port 8443) on the peer which is specifically 
  // in charge of the funtionality of Service Provider of SAML2 SSO profile.
  // then get back <AuthnRequest/>, and
  // send this saml response to IdP
  // -------------------------------------------

  std::string cert_file = "testcert.pem";  //TODO: get from configuration
  std::string privkey_file = "testkey-nopass.pem";
  //TestShib use the certificate for SAML2 SSO
  //std::string ca_file = "cacert_testshib.pem";
  //std::string ca_file = "cacert.pem";
  std::string ca_dir = "./certificates";

  Arc::MCCConfig cfg;
  if (!cert_file.empty())
    cfg.AddCertificate(cert_file);
  if (!privkey_file.empty())
    cfg.AddPrivateKey(privkey_file);
  //if (!ca_file.empty())
  //  cfg.AddCAFile(ca_file);
  if (!ca_dir.empty())
    cfg.AddCADir(ca_dir);

  std::string service_url_str("https://127.0.0.1:8443");
  Arc::URL service_url(service_url_str);

  Arc::ClientHTTP clientSP(cfg, service_url.Host(), service_url.Port(), service_url.Protocol() == "https" ? 1:0, "");
  Arc::PayloadRaw requestSP;
  Arc::PayloadRawInterface *responseSP = NULL;
  Arc::HTTPClientInfo infoSP;
  Arc::MCC_Status statusSP = clientSP.process("GET", &requestSP, &infoSP, &responseSP);
  if (!responseSP) {
    logger.msg(Arc::ERROR, "Request failed: No response");
    return -1;
  }
  if (!statusSP) {
    logger.msg(Arc::ERROR, "Request failed: Error");
    return -1;
  }

  //Parse the authenRequestUrl from response
  std::string authnRequestUrl(responseSP->Content());
  std::cout<<"autnRequestUrl: "<<authnRequestUrl<<std::endl;

  // -------------------------------------------
  // User-Agent: Send the AuthnRequest to IdP,
  // (IP-based authentication, and Username/Password authentication)
  // get back b64 encoded saml response, and
  // send this saml response to SP
  // -------------------------------------------
  //

  Arc::URL url(authnRequestUrl);
  std::string url_str = url.str();
  size_t pos = url_str.find(":");
  pos = url_str.find(":", pos+1);
  pos = url_str.find("/", pos+1);
  std::string path = url_str.substr(pos);

  Arc::ClientHTTP clientIdP(cfg, url.Host(), url.Port(), url.Protocol() == "https" ? 1:0, path);
  Arc::PayloadRaw requestIdP;
  Arc::PayloadRawInterface *responseIdP = NULL;
  Arc::HTTPClientInfo infoIdP;
  Arc::MCC_Status statusIdP = clientIdP.process("GET", &requestIdP, &infoIdP, &responseIdP);
  if (!responseIdP) {
    logger.msg(Arc::ERROR, "Request failed: No response");
    return -1;
  }
  if (!statusIdP) {
    logger.msg(Arc::ERROR, "Request failed: Error");
    return -1;
  }

  //The following code is for authentication (username/password)
  std::string resp_html;
  Arc::HTTPClientInfo redirect_info = infoIdP;
  do {
    std::cout<<"Code: "<<redirect_info.code<<"Reason: "<<redirect_info.reason<<"Size: "<<
    redirect_info.size<<"Type: "<<redirect_info.type<<"Set-Cookie: "<<redirect_info.cookie<<
    "Location: "<<redirect_info.location<<std::endl;
    if(redirect_info.code != 302) break;

    Arc::URL redirect_url(redirect_info.location);
    Arc::ClientHTTP redirect_client(cfg, redirect_url.Host(), redirect_url.Port(),
                    redirect_url.Protocol() == "https" ? 1:0, redirect_url.Path());

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

  Arc::URL redirect_url_final("https://idp.testshib.org:443/idp/Authn/UserPassword");
  Arc::ClientHTTP redirect_client_final(cfg, redirect_url_final.Host(), redirect_url_final.Port(),
              redirect_url_final.Protocol() == "https" ? 1:0, redirect_url_final.Path());
  Arc::PayloadRaw redirect_request_final;
  std::string login_html("j_username=myself&j_password=myself");
  redirect_request_final.Insert(login_html.c_str(),0,login_html.size());
  std::map<std::string, std::string> http_attributes;
  http_attributes["Content-Type"] = "application/x-www-form-urlencoded";
 
  //Use the first cookie
  if(!(infoIdP.cookie.empty()))
    http_attributes["Cookie"]=infoIdP.cookie;
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
 
  //--------------------------------------------------------
  //Decode/verify/decrypt b64 encoded saml response from IdP
  //One option: put the saml assertion into soap header according 
  //to WS-Security's saml profile
  //Another option: redirect the saml response to SP side
  //--------------------------------------------------------
  Arc::XMLNode html_node(html_body);
  std::string saml_resp = html_node["body"]["form"]["div"]["input"].Attribute("value");
  std::cout<<"SAML Response: "<<saml_resp<<std::endl;

  Arc::XMLNode node;
  //Decode the saml response (b64 format)
  Arc::BuildNodefromMsg(saml_resp, node);
  std::string decoded_saml_resp;
  node.GetXML(decoded_saml_resp);
  std::cout<<"Decoded SAML Response: "<<decoded_saml_resp<<std::endl;

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

  return true;
}

}


