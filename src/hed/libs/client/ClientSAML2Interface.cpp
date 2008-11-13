#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <map>

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/MCC.h>
#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include <arc/xmlsec/saml_util.h>

#include "ClientSAML2Interface.h"

namespace Arc {
  Logger ClientHTTPwithSAML2SSO::logger(Logger::getRootLogger(), "ClientHTTPwithSAML2SSO");
  Logger ClientSOAPwithSAML2SSO::logger(Logger::getRootLogger(), "ClientSOAPwithSAML2SSO");

  ClientHTTPwithSAML2SSO::ClientHTTPwithSAML2SSO(const BaseConfig& cfg, const std::string& host,
			 int port, bool tls, const std::string& path) : http_client_(NULL) {
    if(!(Arc::init_xmlsec())) return;   
    http_client_ = new ClientHTTP(cfg, host, port, tls, path); 
  }

  ClientHTTPwithSAML2SSO::~ClientHTTPwithSAML2SSO() { 
    Arc::final_xmlsec(); 
    if(http_client_) delete http_client_;
  }

  static MCC_Status process_saml2sso(const std::string& idp_name, ClientHTTP* http_client, Logger& logger) {

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
  //

    Arc::PayloadRaw requestSP;
    Arc::PayloadRawInterface *responseSP = NULL;
    Arc::HTTPClientInfo infoSP;
    //std::string idp_name("https://idp.testshib.org/idp/shibboleth"); //TODO
    requestSP.Insert(idp_name.c_str(),0, idp_name.size());
    //Call the peer http endpoint with path "saml2sp", which
    //is the endpoint of saml SP service
    Arc::MCC_Status statusSP = http_client->process("POST", "saml2sp",&requestSP, &infoSP, &responseSP);
    if (!responseSP) {
      logger.msg(Arc::ERROR, "Request failed: No response from SPService");
      return MCC_Status();
    }
    if (!statusSP) {
      logger.msg(Arc::ERROR, "Request failed: Error");
      if(responseSP) delete responseSP;
      return MCC_Status();
    }

    std::string cert_file_ = "testcert.pem";  //TODO: get from configuration
    std::string privkey_file_ = "testkey-nopass.pem";
    //TestShib use the certificate for SAML2 SSO
    std::string ca_file_ = "cacert_testshib.pem";
    std::string ca_dir_ = "./certificates";

    //Parse the authenRequestUrl from response
    std::string authnRequestUrl(responseSP->Content());
    logger.msg(Arc::VERBOSE, "Authentication Request URL: %s", authnRequestUrl.c_str());

    if(responseSP) delete responseSP;

    // -------------------------------------------
    //User-Agent: Send the AuthnRequest to IdP,
    //(IP-based authentication, and Username/Password authentication)
    //get back b64 encoded saml response, and
    //send this saml response to SP
    //-------------------------------------------

    Arc::URL url(authnRequestUrl);
    std::string url_str = url.str();
    size_t pos = url_str.find(":");
    pos = url_str.find(":", pos+1);
    pos = url_str.find("/", pos+1);
    std::string idp_path(url_str.substr(pos));
 
    Arc::MCCConfig cfg;
    if (!cert_file_.empty())
      cfg.AddCertificate(cert_file_);
    if (!privkey_file_.empty())
      cfg.AddPrivateKey(privkey_file_);
    if (!ca_file_.empty())
      cfg.AddCAFile(ca_file_);
    if (!ca_dir_.empty())
      cfg.AddCADir(ca_dir_);

    ClientHTTP clientIdP(cfg, url.Host(), url.Port(), url.Protocol() == "https" ? 1:0, idp_path);

    Arc::PayloadRaw requestIdP;
    Arc::PayloadRawInterface *responseIdP = NULL;
    Arc::HTTPClientInfo infoIdP;
    Arc::MCC_Status statusIdP = clientIdP.process("GET", &requestIdP, &infoIdP, &responseIdP);
    if (!responseIdP) {
      logger.msg(Arc::ERROR, "Request failed: No response from IdP");
      return MCC_Status();
    }
    if (!statusIdP) {
      logger.msg(Arc::ERROR, "Request failed: Error");
      if(responseIdP) delete responseIdP;
      return MCC_Status();
    }
    if(responseIdP) delete responseIdP;

    //The following code is for authentication (username/password)
    std::string resp_html;
    Arc::HTTPClientInfo redirect_info = infoIdP;
    do {
      /*std::cout<<"Code: "<<redirect_info.code<<"Reason: "<<redirect_info.reason<<"Size: "<<
      redirect_info.size<<"Type: "<<redirect_info.type<<"Set-Cookie: "<<redirect_info.cookie<<
      "Location: "<<redirect_info.location<<std::endl;*/
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
        logger.msg(Arc::ERROR, "Request failed: No response from IdP when doing redirecting");
        return MCC_Status();
      }
      if (!redirect_status) {
        logger.msg(Arc::ERROR, "Request failed: Error");
        if(redirect_response) delete redirect_response;
        return MCC_Status();
      }
      char* content = redirect_response->Content();
      if(content!=NULL) {
        resp_html.assign(redirect_response->Content());
        size_t pos = resp_html.find("j_username");
        if(pos!=std::string::npos) { if(redirect_response) delete redirect_response; break; }
      }
      if(redirect_response) delete redirect_response;
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
      logger.msg(Arc::ERROR, "Request failed: No response from IdP when doing authentication");
      return MCC_Status();
    }
    if (!redirect_status_final) {
      logger.msg(Arc::ERROR, "Request failed: Error");
      if(redirect_response_final) delete redirect_response_final;
      return MCC_Status();
    }

    std::string html_body;
    for(int i = 0;;i++) {
      char* buf = redirect_response_final->Buffer(i);
      if(buf == NULL) break;
      html_body.append(redirect_response_final->Buffer(i), redirect_response_final->BufferSize(i));
      //std::cout<<"Buffer: "<<buf<<std::endl;
    }
    if(redirect_response_final) delete redirect_response_final;

    std::string type = redirect_info_final.type;
    size_t pos1 = type.find(';');
    if (pos1 != std::string::npos)
      type = type.substr(0, pos1);
    if(type != "text/html")
      std::cerr<<"The html response from IdP is with wrong format"<<std::endl;

    Arc::XMLNode html_node(html_body);
    std::string saml_resp = html_node["body"]["form"]["div"]["input"].Attribute("value");
    //std::cout<<"SAML Response: "<<saml_resp<<std::endl;
    Arc::XMLNode samlresp_nd;
    //Decode the saml response (b64 format)
    Arc::BuildNodefromMsg(saml_resp, samlresp_nd);
    std::string decoded_saml_resp;
    samlresp_nd.GetXML(decoded_saml_resp);
    //std::cout<<"Decoded SAML Response: "<<decoded_saml_resp<<std::endl;

    //Verify the signature of saml response
    std::string idname = "ID";
    Arc::XMLSecNode sec_samlresp_nd(samlresp_nd);
    //Since the certificate from idp.testshib.org which signs the saml response is self-signed
    //certificate, only check the signature here.
    if(sec_samlresp_nd.VerifyNode(idname,"", "", false)) {
      logger.msg(Arc::INFO, "Succeeded to verify the signature under <samlp:Response/>");
    }
    else {
      logger.msg(Arc::ERROR, "Failed to verify the signature under <samlp:Response/>");
    }

    //Send the encrypted saml assertion to service side through this main message chain
    //Get the encrypted saml assertion in this saml response
    Arc::XMLNode assertion_nd = samlresp_nd["saml:EncryptedAssertion"];
    std::string saml_assertion;
    assertion_nd.GetXML(saml_assertion);
    //std::cout<<"Encrypted saml assertion: "<<saml_assertion<<std::endl;
    requestSP.Truncate(0);
    requestSP.Insert(saml_assertion.c_str(),0, saml_assertion.size());
    statusSP = http_client->process("POST", "saml2sp", &requestSP, &infoSP, &responseSP);
    if (!responseSP) {
      logger.msg(Arc::ERROR, "Request failed: No response from SP Service when sending saml assertion to SP");
      return MCC_Status();
    }
    if (!statusSP) {
      logger.msg(Arc::ERROR, "Request failed: Error");
      if(responseSP) delete responseSP;
      return MCC_Status();
    }
    if(responseSP) delete responseSP;

    //Arc::PayloadRawInterface *responseService = NULL;
    //Arc::HTTPClientInfo infoService;
 

    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  MCC_Status ClientHTTPwithSAML2SSO::process(const std::string& method,
                                 PayloadRawInterface *request,
                                 HTTPClientInfo *info,
                                 PayloadRawInterface **response) {
    return(process(method, "", request, info, response));
  }

  MCC_Status ClientHTTPwithSAML2SSO::process(const std::string& method,
                                 const std::string& path,
                                 PayloadRawInterface *request,
                                 HTTPClientInfo *info,
                                 PayloadRawInterface **response) {
    //Do the saml2sso
    std::string idp_name("https://idp.testshib.org/idp/shibboleth");
    Arc::MCC_Status status = process_saml2sso(idp_name, http_client_, logger);
    if(!status) {
      logger.msg(Arc::ERROR, "SAML2SSO process failed");
      return MCC_Status();
    }
    //Send the real message
    status = http_client_->process(method, path , request, info, response);             
    return status;
  }

  ClientSOAPwithSAML2SSO::ClientSOAPwithSAML2SSO(const BaseConfig& cfg, const std::string& host, int port,
               bool tls, const std::string& path) : soap_client_(NULL) {
    if(!(Arc::init_xmlsec())) return;
    soap_client_ = new ClientSOAP(cfg, host, port, tls, path);
  }
  
  ClientSOAPwithSAML2SSO::~ClientSOAPwithSAML2SSO() {
    Arc::final_xmlsec();
    if(soap_client_) delete soap_client_;
  }
  
  MCC_Status ClientSOAPwithSAML2SSO::process(PayloadSOAP *request, PayloadSOAP **response) {
    return process("", request, response); 
  }
  
  MCC_Status ClientSOAPwithSAML2SSO::process(const std::string& action, PayloadSOAP *request,
                       PayloadSOAP **response) {
    //Do the saml2sso
    ClientHTTP* http_client = dynamic_cast<ClientHTTP*>(soap_client_);
    std::string idp_name("https://idp.testshib.org/idp/shibboleth");
    Arc::MCC_Status status = process_saml2sso(idp_name, http_client, logger);
    if(!status) {
      logger.msg(Arc::ERROR, "SAML2SSO process failed");
      return MCC_Status();
    }
    //Send the real message
    status = soap_client_->process(action, request, response);
    return status;
  }


} // namespace Arc
