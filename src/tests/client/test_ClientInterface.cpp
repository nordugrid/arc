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
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  std::string url_str("https://127.0.0.1:60000/echo");
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  mcc_cfg.AddPrivateKey("../echo/testkey-nopass.pem");
  mcc_cfg.AddCertificate("../echo/testcert.pem");
  mcc_cfg.AddCAFile("../echo/testcacert.pem");
  mcc_cfg.AddCADir("../echo/certificates");

  Arc::NS echo_ns; echo_ns["echo"]="urn:echo";

  //Create a SOAP client
  logger.msg(Arc::INFO, "Creating a soap client");

  
  //Configuration for the security handler of usernametoken and x509token
  Arc::XMLNode sechanlder_nd_ut("\
        <SecHandler name='usernametoken.handler' id='usernametoken' event='outgoing'>\
            <Process>generate</Process>\
            <PasswordEncoding>digest</PasswordEncoding>\
            <Username>user</Username>\
            <Password>passwd</Password>\
        </SecHandler>");

  Arc::XMLNode sechanlder_nd_xt("\
        <SecHandler name='x509token.handler' id='x509token' event='outgoing'>\
            <Process>generate</Process>\
            <CertificatePath>./testcert.pem</CertificatePath>\
            <KeyPath>./testkey-nopass.pem</KeyPath>\
        </SecHandler>");

  Arc::ClientSOAP *client;
  client = new Arc::ClientSOAP(mcc_cfg,url);

  client->AddSecHandler(sechanlder_nd_ut, "arcshc");
  //client->AddSecHandler(sechanlder_nd_xt, "arcshc");

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

  return 0;
}
