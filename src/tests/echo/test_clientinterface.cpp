#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>
#include <stdexcept>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
//#include "../../hed/libs/client/ClientSAML2Interface.h"
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  //Create a SOAP client
  logger.msg(Arc::INFO, "Creating a soap client");

  std::string url_str("https://127.0.0.1:60000:/echo");
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  mcc_cfg.AddPrivateKey("testkey-nopass.pem");
  mcc_cfg.AddCertificate("testcert.pem");
  mcc_cfg.AddCAFile("cacert.pem");

  //Arc::WSSInfo wssinfo;
  //wssinfo.username="user";
  //wssinfo.password="passwd";
  //wssinfo.password_encoding="digest";
  //mcc_cfg.AddWSSType(Arc::USERNAMETOKEN);
  //mcc_cfg.AddWSSInfo(wssinfo);

  Arc::NS echo_ns; echo_ns["echo"]="urn:echo";

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

#if 0
  Arc::ClientHTTPwithSAML2SSO *client_http;
  client_http = new Arc::ClientHTTPwithSAML2SSO(mcc_cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path());
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::PayloadRaw req_http;
  //req_http.Insert();

  Arc::PayloadRawInterface* resp_http = NULL;
  Arc::HTTPClientInfo info;

  if(client_http) {
    Arc::MCC_Status status = client_http->process("GET", "echo", &req_http,&info,&resp_http);
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


  Arc::ClientSOAPwithSAML2SSO *client_soap;
  client_soap = new Arc::ClientSOAPwithSAML2SSO(mcc_cfg,url);
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::PayloadSOAP req_soap(echo_ns);
  req_soap.NewChild("echo").NewChild("say")="HELLO";

  Arc::PayloadSOAP* resp_soap = NULL;

  if(client_soap) {
    Arc::MCC_Status status = client_soap->process(&req_soap,&resp_soap);
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

  return 0;
}
