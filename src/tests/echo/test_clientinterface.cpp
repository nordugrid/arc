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

  std::string url_str("https://127.0.0.1:60000");
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  mcc_cfg.AddPrivateKey("key.pem");
  mcc_cfg.AddCertificate("cert.pem");
  mcc_cfg.AddCAFile("ca.pem");

  Arc::ClientSOAP *client;
  client = new Arc::ClientSOAP(mcc_cfg,url.Host(),url.Port(),url.Protocol() == "https",url.Path(), Arc::ClientSOAP::UsernameToken);

  // Create and send echo request
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::NS echo_ns; echo_ns["echo"]="urn:echo";
  Arc::PayloadSOAP req(echo_ns);
  req.NewChild("echo").NewChild("say")="HELLO";

  Arc::PayloadSOAP* resp = NULL;

  if(client) {
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

  return 0;
}
