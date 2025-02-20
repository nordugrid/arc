// -*- indent-tabs-mode: nil -*-

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

#include "ClientSAML2SSO.h"

namespace Arc {
  Logger ClientHTTPwithSAML2SSO::logger(Logger::getRootLogger(), "ClientHTTPwithSAML2SSO");
  Logger ClientSOAPwithSAML2SSO::logger(Logger::getRootLogger(), "ClientSOAPwithSAML2SSO");

  ClientHTTPwithSAML2SSO::ClientHTTPwithSAML2SSO(const BaseConfig& cfg,
                                                 const URL& url)
    : http_client_(NULL),
      authn_(false) {

    http_client_ = new ClientHTTP(cfg, url, 60);
    //Use the credential and trusted certificates from client's main chain to
    //contact IdP
    cert_file_ = cfg.cert;
    privkey_file_ = cfg.key;
    ca_file_ = cfg.cafile;
    ca_dir_ = cfg.cadir;
  }

  ClientHTTPwithSAML2SSO::~ClientHTTPwithSAML2SSO() {
    if (http_client_)
      delete http_client_;
  }

  static MCC_Status process_saml2sso(const std::string& idp_name, const std::string& username,
                                     const std::string& password, ClientHTTP *http_client, std::string& cert_file,
                                     std::string& privkey_file, std::string& ca_file, std::string& ca_dir, Logger& logger,
                                     std::string& cookie) {

    // -------------------------------------------
    // User-Agent: 1. Send an empty http request to SP;
    //
    // The saml2sso process share the same tcp/tls connection (on the service 
    // side, SP service and the functional/real service which is to be protected 
    // are supposed to be at the same service chain) with the
    // main client chain. And because of this connection sharing, if the
    // saml2sso process (interaction between user-agent/SP service/extenal IdP
    // service, see SAML2 SSO profile) is succeeded, we can suppose that the
    // later real client/real service interaction is authorized.
    //
    // 2. User-Agent then get back <AuthnRequest/>, and send it response to IdP
    //
    // SP Service: a service based on http service on the service side, which is
    // specifically in charge of the funtionality of Service Provider of SAML2
    // SSO profile.
    //
    // User-Agent: Since the SAML2 SSO profile is web-browser based, so here we
    // implement the code which is with the same functionality as browser's user agent.
    // -------------------------------------------
    //

    Arc::PayloadRaw requestSP;
    Arc::PayloadRawInterface *responseSP = NULL;
    Arc::HTTPClientInfo infoSP;
    requestSP.Insert(idp_name.c_str(), 0, idp_name.size());
    //Call the peer http endpoint with path "saml2sp", which
    //is the endpoint of saml SP service
    Arc::MCC_Status statusSP = http_client->process("POST", "/saml2sp", &requestSP, &infoSP, &responseSP);
    if (!responseSP) {
      logger.msg(Arc::ERROR, "Request failed: No response from SPService");
      return MCC_Status();
    }
    if (!statusSP) {
      logger.msg(Arc::ERROR, "Request failed: response from SPService is not as expected");
      if (responseSP)
        delete responseSP;
      return MCC_Status();
    }

    //Parse the authenRequestUrl from response
    std::string authnRequestUrl(responseSP->Content());
    logger.msg(DEBUG, "Authentication Request URL: %s", authnRequestUrl);

    if (responseSP)
      delete responseSP;

    // -------------------------------------------
    //User-Agent: Send the AuthnRequest to IdP,
    //(IP-based authentication, and Username/Password authentication)
    //get back b64 encoded saml response, and
    //send this saml response to SP
    //-------------------------------------------

    Arc::URL url(authnRequestUrl);

    Arc::MCCConfig cfg;
    if (!cert_file.empty())
      cfg.AddCertificate(cert_file);
    if (!privkey_file.empty())
      cfg.AddPrivateKey(privkey_file);
    if (!ca_file.empty())
      cfg.AddCAFile(ca_file);
    if (!ca_dir.empty())
      cfg.AddCADir(ca_dir);

    ClientHTTP clientIdP(cfg, url);

    Arc::PayloadRaw requestIdP;
    Arc::PayloadRawInterface *responseIdP = NULL;
    Arc::HTTPClientInfo infoIdP;

    std::multimap<std::string, std::string> http_attributes;
    if (!(cookie.empty()))
      http_attributes.insert(std::pair<std::string,std::string>("Cookie",cookie));

    //Contact the IdP service
    Arc::MCC_Status statusIdP = clientIdP.process("GET", http_attributes, &requestIdP, &infoIdP, &responseIdP);

    if ((!(infoIdP.cookies.empty())) && infoIdP.code != 200)
      cookie = *(infoIdP.cookies.begin());

    if (!responseIdP) {
      logger.msg(Arc::ERROR, "Request failed: No response from IdP");
      return MCC_Status();
    }
    if (!statusIdP) {
      logger.msg(Arc::ERROR, "Request failed: response from SPService is not as expected");
      if (responseIdP)
        delete responseIdP;
      return MCC_Status();
    }
    //Record the returned html content in case something is wrong
    std::string html_content;
    if(responseIdP->Content() != NULL)
      html_content = responseIdP->Content();
    if (responseIdP)
      delete responseIdP;

    //The following code is for authentication (username/password)
    std::string resp_html;
    Arc::HTTPClientInfo redirect_info = infoIdP;

    if(redirect_info.code!= 200) {
      int count = 0;
      do {
        /*
        std::cout<<"Code: "<<redirect_info.code<<"  Reason: "<<redirect_info.reason<<"  Size: "<<
           redirect_info.size<<"  Type: "<<redirect_info.type<<"  Set-Cookie: "<<(redirect_info.cookies.empty()?"":(*(redirect_info.cookies.begin())))<< "  Location: "<<redirect_info.location<<std::endl;
        */

        //"break" if the response is not "redirection"
        if (redirect_info.code != 302)
          break;

        Arc::URL redirect_url(redirect_info.location);
        Arc::ClientHTTP redirect_client(cfg, redirect_url);

        Arc::PayloadRaw redirect_request;
        Arc::PayloadRawInterface *redirect_response = NULL;

        std::multimap<std::string, std::string> http_attributes;

        if (!(cookie.empty()))
          http_attributes.insert(std::pair<std::string,std::string>("Cookie",cookie));

        //Keep contacting IdP
        Arc::MCC_Status redirect_status = redirect_client.process("GET", http_attributes,
                                                                &redirect_request, &redirect_info, &redirect_response);

        if (!(redirect_info.cookies.empty()))
          cookie = *(redirect_info.cookies.begin());
      
        if (!redirect_response) {
          logger.msg(Arc::ERROR, "Request failed: No response from IdP when doing redirecting");
          return MCC_Status();
        }
        if (!redirect_status) {
          logger.msg(Arc::ERROR, "Request failed: response from IdP is not as expected when doing redirecting");
          if (redirect_response)
            delete redirect_response;
          return MCC_Status();
        }
        char *content = redirect_response->Content();
        if (content != NULL) {
          resp_html.assign(redirect_response->Content());
          size_t pos = resp_html.find("j_username");
          if (pos != std::string::npos) {
            if (redirect_response)
              delete redirect_response;
            break;  
            //"break" if the "j_username" is found in response, 
            //here for different implentation of IdP, different 
            //name could be searched.
          }
        }
        if (redirect_response)
          delete redirect_response;
        count++;
        if (count > 5)
          break;             //At most loop 5 times
      } while (1);



      //Arc::URL redirect_url_final("https://idp.testshib.org:443/idp/Authn/UserPassword");
      Arc::URL redirect_url_final(infoIdP.location);
      Arc::ClientHTTP redirect_client_final(cfg, redirect_url_final);
      Arc::PayloadRaw redirect_request_final;

      //std::string login_html("j_username=myself&j_password=myself");

      std::multimap<std::string, std::string> http_attributes2;

      std::string login_html;
      login_html.append("j_username=").append(username).append("&j_password=").append(password);

      redirect_request_final.Insert(login_html.c_str(), 0, login_html.size());

      http_attributes2.insert(std::pair<std::string,std::string>("Content-Type","application/x-www-form-urlencoded"));

      if(!(cookie.empty()))
        http_attributes2.insert(std::pair<std::string,std::string>("Cookie",cookie));

      Arc::PayloadRawInterface *redirect_response_final = NULL;
      Arc::HTTPClientInfo redirect_info_final;
  
      //Contact IdP to send the username/password
      Arc::MCC_Status redirect_status_final = redirect_client_final.process("POST", http_attributes2,
                                      &redirect_request_final, &redirect_info_final, &redirect_response_final);

      if (!(redirect_info_final.cookies.empty()))
        cookie = *(redirect_info_final.cookies.begin());

      if (!redirect_response_final) {
        logger.msg(Arc::ERROR, "Request failed: No response from IdP when doing authentication");
        return MCC_Status();
      }
      if (!redirect_status_final) {
        logger.msg(Arc::ERROR, "Request failed: response from IdP is not as expected when doing authentication");
        if (redirect_response_final)
          delete redirect_response_final;
        return MCC_Status();
      }

      std::string html_body;
      for (int i = 0;; i++) {
      char *buf = redirect_response_final->Buffer(i);
        if (buf == NULL)
          break;
        html_body.append(redirect_response_final->Buffer(i), redirect_response_final->BufferSize(i));
        //std::cout<<"Buffer: "<<buf<<std::endl;
      }
      if (redirect_response_final)
        delete redirect_response_final;

      std::string type = redirect_info_final.type;
      size_t pos1 = type.find(';');
      if (pos1 != std::string::npos)
        type = type.substr(0, pos1);
      if (type != "text/html")
        std::cerr << "The html response from IdP is with wrong format" << std::endl;


      Arc::XMLNode html_node(html_body);
      //Get the samlp:Response from responded html message.
      //here for different implementation of IdP, the responded html 
      //messages could have different structures.
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
      //certificate, only check the signature here, while do not check the whole chain of 
      //certificates
      if (sec_samlresp_nd.VerifyNode(idname, "", "", false, false))
        logger.msg(Arc::INFO, "Succeeded to verify the signature under <samlp:Response/>");
      else
        logger.msg(Arc::ERROR, "Failed to verify the signature under <samlp:Response/>");


      //Send the encrypted saml assertion to service side through this main message chain
      //Get the encrypted saml assertion in this saml response
      Arc::XMLNode assertion_nd = samlresp_nd["EncryptedAssertion"];
      std::string saml_assertion;
      assertion_nd.GetXML(saml_assertion);
      //std::cout<<"Encrypted saml assertion: "<<saml_assertion<<std::endl;
      requestSP.Truncate(0);
      requestSP.Insert(saml_assertion.c_str(), 0, saml_assertion.size());
      statusSP = http_client->process("POST", "/saml2sp", &requestSP, &infoSP, &responseSP);

      if (!responseSP) {
        logger.msg(Arc::ERROR, "Request failed: No response from SP Service when sending SAML assertion to SP");
        return MCC_Status();
      }
      if (!statusSP) {
        logger.msg(Arc::ERROR, "Request failed: response from SP Service is not as expected when sending SAML assertion to SP");
        if (responseSP)
          delete responseSP;
        return MCC_Status();
      }
      if (responseSP)
        delete responseSP;

    } else { 
      // if "redirect_info.code == 200", then something could be wrong,
      // because "200" is not supposed to appear in this step
      logger.msg(Arc::ERROR,"IdP return some error message: %s", html_content.c_str());
      return MCC_Status();
    }

    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  MCC_Status ClientHTTPwithSAML2SSO::process(const std::string& method,
                                             PayloadRawInterface *request,
                                             HTTPClientInfo *info,
                                             PayloadRawInterface **response, const std::string& idp_name,
                                             const std::string& username, const std::string& password, 
                                             const bool reuse_authn) {
    return (process(method, "", request, info, response, idp_name, username, password, reuse_authn));
  }

  MCC_Status ClientHTTPwithSAML2SSO::process(const std::string& method,
                                             const std::string& path,
                                             PayloadRawInterface *request,
                                             HTTPClientInfo *info,
                                             PayloadRawInterface **response, const std::string& idp_name,
                                             const std::string& username, const std::string& password, 
                                             const bool reuse_authn) {
    if (!authn_) { //If has not yet passed the saml2sso process
      //Do the saml2sso
      Arc::MCC_Status status = process_saml2sso(idp_name, username, password,
                                                http_client_, cert_file_, privkey_file_, ca_file_, ca_dir_, logger, cookie);
      if (!status) {
        logger.msg(Arc::ERROR, "SAML2SSO process failed");
        return MCC_Status();
      }
      if(reuse_authn) authn_ = true; //Reuse or not reuse the result from saml2sso
    }
    //Send the real message
    Arc::MCC_Status status = http_client_->process(method, path, request, info, response);
    return status;
  }

  ClientSOAPwithSAML2SSO::ClientSOAPwithSAML2SSO(const BaseConfig& cfg,
                                                 const URL& url)
    : soap_client_(NULL),
      authn_(false) {
    soap_client_ = new ClientSOAP(cfg, url);
    //Use the credential and trusted certificates from client's main chain to
    //contact IdP
    cert_file_ = cfg.cert;
    privkey_file_ = cfg.key;
    ca_file_ = cfg.cafile;
    ca_dir_ = cfg.cadir;
  }

  ClientSOAPwithSAML2SSO::~ClientSOAPwithSAML2SSO() {
    if (soap_client_)
      delete soap_client_;
  }

  MCC_Status ClientSOAPwithSAML2SSO::process(PayloadSOAP *request, PayloadSOAP **response,
                                             const std::string& idp_name, const std::string& username, 
                                             const std::string& password, const bool reuse_authn) {
    return process("", request, response, idp_name, username, password, reuse_authn);
  }

  MCC_Status ClientSOAPwithSAML2SSO::process(const std::string& action, PayloadSOAP *request,
                                             PayloadSOAP **response, const std::string& idp_name,
                                             const std::string& username, const std::string& password, 
                                             const bool reuse_authn) {
    //Do the saml2sso
    if (!authn_) { //If has not yet passed the saml2sso process
      ClientHTTP *http_client = dynamic_cast<ClientHTTP*>(soap_client_);
      Arc::MCC_Status status = process_saml2sso(idp_name, username, password,
                                                http_client, cert_file_, privkey_file_, 
                                                ca_file_, ca_dir_, logger, cookie);
      if (!status) {
        logger.msg(Arc::ERROR, "SAML2SSO process failed");
        return MCC_Status();
      }
      if(reuse_authn) authn_ = true;  //Reuse or not reuse the result from saml2sso
    }
    //Send the real message
    Arc::MCC_Status status = soap_client_->process(action, request, response);
    return status;
  }

} // namespace Arc
