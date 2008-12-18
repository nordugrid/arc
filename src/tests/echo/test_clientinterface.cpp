#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>
#include <stdexcept>

#include <arc/GUID.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/ClientSAML2SSO.h>
#include <arc/client/ClientX509Delegation.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  /************************************************/

  std::string url_str("https://127.0.0.1:60000/echo");
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  mcc_cfg.AddPrivateKey("testkey-nopass.pem");
  mcc_cfg.AddCertificate("testcert.pem");
  mcc_cfg.AddCAFile("cacert.pem");
  mcc_cfg.AddCADir("certificates");

  //Arc::WSSInfo wssinfo;
  //wssinfo.username="user";
  //wssinfo.password="passwd";
  //wssinfo.password_encoding="digest";
  //mcc_cfg.AddWSSType(Arc::USERNAMETOKEN);
  //mcc_cfg.AddWSSInfo(wssinfo);

  Arc::NS echo_ns; echo_ns["echo"]="urn:echo";

#if 0
  /******** Test to service **********/
  //Create a SOAP client
  logger.msg(Arc::INFO, "Creating a soap client");

  Arc::ClientSOAP *client;
  client = new Arc::ClientSOAP(mcc_cfg,url);

  // Create and send echo request
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::PayloadSOAP req(echo_ns);
  req.NewChild("echo").NewChild("say")="HELLO";

  Arc::PayloadSOAP* resp = NULL;

  if(client) {
    std::string str;
    req.GetXML(str);
    std::cout<<"request: "<<str<<std::endl;
    Arc::MCC_Status status = client->process(&req,&resp);
    if(!status) {
      logger.msg(Arc::ERROR, "SOAP invokation failed");
      throw std::runtime_error("SOAP invokation failed");
    }
    if(resp == NULL) {
      logger.msg(Arc::ERROR,"There was no SOAP response");
      throw std::runtime_error("There was no SOAP response");
    }
  }

  std::string xml;
  resp->GetXML(xml);
  std::cout << "XML: "<< xml << std::endl;
  std::cout << "Response: " << (std::string)((*resp)["echoResponse"]["hear"]) << std::endl;

  if(resp) delete resp;
  if(client) delete client;
#endif

#if 0
  std::string idp_name = "https://idp.testshib.org/idp/shibboleth";

  /******** Test to service with SAML2SSO **********/
  //Create a HTTP client
  logger.msg(Arc::INFO, "Creating a http client");
  
  std::string username = "myself";
  std::string password = "myself";

  Arc::ClientHTTPwithSAML2SSO *client_http;
  client_http = new Arc::ClientHTTPwithSAML2SSO(mcc_cfg,url);
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::PayloadRaw req_http;
  //req_http.Insert();

  Arc::PayloadRawInterface* resp_http = NULL;
  Arc::HTTPClientInfo info;

  if(client_http) {
    Arc::MCC_Status status = client_http->process("GET", "echo", &req_http,&info,&resp_http, idp_name, username, password);
    if(!status) {
      logger.msg(Arc::ERROR, "HTTP with SAML2SSO invokation failed");
      throw std::runtime_error("HTTP with SAML2SSO invokation failed");
    }
    if(resp_http == NULL) {
      logger.msg(Arc::ERROR,"There was no HTTP response");
      throw std::runtime_error("There was no HTTP response");
    }
  }

  if(resp_http) delete resp_http;
  if(client_http) delete client_http;



  //Create a SOAP client
  logger.msg(Arc::INFO, "Creating a soap client");
 
  Arc::ClientSOAPwithSAML2SSO *client_soap;
  client_soap = new Arc::ClientSOAPwithSAML2SSO(mcc_cfg,url);
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::PayloadSOAP req_soap(echo_ns);
  req_soap.NewChild("echo").NewChild("say")="HELLO";

  Arc::PayloadSOAP* resp_soap = NULL;

  if(client_soap) {
    Arc::MCC_Status status = client_soap->process(&req_soap,&resp_soap, idp_name, username, password);
    if(!status) {
      logger.msg(Arc::ERROR, "SOAP with SAML2SSO invokation failed");
      throw std::runtime_error("SOAP with SAML2SSO invokation failed");
    }
    if(resp_soap == NULL) {
      logger.msg(Arc::ERROR,"There was no SOAP response");
      throw std::runtime_error("There was no SOAP response");
    }
  }
  std::string xml_soap;
  resp_soap->GetXML(xml_soap);
  std::cout << "XML: "<< xml_soap << std::endl;
  std::cout << "Response: " << (std::string)((*resp_soap)["echoResponse"]["hear"]) << std::endl;

  if(resp_soap) delete resp_soap;
  if(client_soap) delete client_soap;
#endif

  /******** Test to ARC delegation service **********/
  std::string arc_deleg_url_str("https://127.0.0.1:60000/delegation");
  Arc::URL arc_deleg_url(arc_deleg_url_str);
  Arc::MCCConfig arc_deleg_mcc_cfg;
  arc_deleg_mcc_cfg.AddPrivateKey("userkey-nopass.pem");
  arc_deleg_mcc_cfg.AddCertificate("usercert.pem");
  //arc_deleg_mcc_cfg.AddCAFile("cacert.pem");
  arc_deleg_mcc_cfg.AddCADir("certificates");
  //Create a delegation SOAP client 
  logger.msg(Arc::INFO, "Creating a delegation soap client");
  Arc::ClientX509Delegation *arc_deleg_client = NULL;
  arc_deleg_client = new Arc::ClientX509Delegation(arc_deleg_mcc_cfg, arc_deleg_url);
  std::string arc_delegation_id;
  if(arc_deleg_client) {
    if(!(arc_deleg_client->createDelegation(Arc::DELEG_ARC, arc_delegation_id))) {
      logger.msg(Arc::ERROR, "Delegation to ARC delegation service failed");
      throw std::runtime_error("Delegation to ARC delegation service failed");
    }
  }
  logger.msg(Arc::INFO, "Delegation ID: %s", arc_delegation_id.c_str());
  if(arc_deleg_client) delete arc_deleg_client;  

  /******** Test to gridsite delegation service **********/
  std::string gs_deleg_url_str("https://cream.grid.upjs.sk:8443/ce-cream/services/gridsite-delegation");
  Arc::URL gs_deleg_url(gs_deleg_url_str);
  Arc::MCCConfig gs_deleg_mcc_cfg;
  gs_deleg_mcc_cfg.AddProxy("x509up_u126587");
  //gs_deleg_mcc_cfg.AddPrivateKey("userkey-nopass.pem");
  //gs_deleg_mcc_cfg.AddCertificate("usercert.pem");
  gs_deleg_mcc_cfg.AddCADir("certificates");
  //Create a delegation SOAP client
  logger.msg(Arc::INFO, "Creating a delegation soap client");
  Arc::ClientX509Delegation *gs_deleg_client = NULL;
  gs_deleg_client = new Arc::ClientX509Delegation(gs_deleg_mcc_cfg, gs_deleg_url);
  std::string gs_delegation_id;
  gs_delegation_id = Arc::UUID();
  if(gs_deleg_client) {
    if(!(gs_deleg_client->createDelegation(Arc::DELEG_GRIDSITE, gs_delegation_id))) {
      logger.msg(Arc::ERROR, "Delegation to gridsite delegation service failed");
      throw std::runtime_error("Delegation to gridsite delegation service failed");
    }
  }
  logger.msg(Arc::INFO, "Delegation ID: %s", gs_delegation_id.c_str());
  if(gs_deleg_client) delete gs_deleg_client;

  return 0;
}
