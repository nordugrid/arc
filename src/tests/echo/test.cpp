#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/client/ClientInterface.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr1(std::cerr, "C");
  Arc::Logger::rootLogger.addDestination(logcerr1);
  Arc::LogStream logcerr2(std::cerr, "sv_SE.UTF8");
  Arc::Logger::rootLogger.addDestination(logcerr2);
  // Load service chain
  logger.msg(Arc::INFO, "Creating service side chain");
  Arc::Config service_config("service.xml");
  if(!service_config) {
    logger.msg(Arc::ERROR, "Failed to load service configuration");
    return -1;
  };
  Arc::MCCLoader service_loader(service_config);
  logger.msg(Arc::INFO, "Service side MCCs are loaded");

  logger.msg(Arc::INFO, "Creating client interface");
  Arc::MCCConfig client_cfg;
  // Paths to plugins in source tree
  client_cfg.AddPluginsPath("../../hed/mcc/http/.libs");
  client_cfg.AddPluginsPath("../../hed/mcc/soap/.libs");
  client_cfg.AddPluginsPath("../../hed/mcc/tls/.libs");
  client_cfg.AddPluginsPath("../../hed/mcc/tcp/.libs");
  client_cfg.AddPluginsPath("../../hed/shc/.libs");
  // Specify credentials
  client_cfg.AddPrivateKey("./testkey-nopass.pem");
  client_cfg.AddCertificate("./testcert.pem");
  client_cfg.AddCAFile("./testcacert.pem");
  // Create client instance for contacting echo service
  Arc::ClientSOAP client(client_cfg,Arc::URL("https://127.0.0.1:60000/echo"));
  // Add SecHandler to chain at TLS location to accept only
  // connection to server with specified DN
  std::list<std::string> dns;
  // Making a list of allowed hosts. To make this test
  // fail change DN below.
  dns.push_back("/O=Grid/O=Test/CN=localhost");
  // Creating SecHandler configuration with allowed DNs
  Arc::DNListHandlerConfig dncfg(dns,"outgoing");
  // Adding SecHandler to client at TLS level.
  // We have to explicitely specify method of ClientTCP 
  // class to attach SecHandler at proper location.
  client.Arc::ClientTCP::AddSecHandler(dncfg,Arc::TLSSec); 
  logger.msg(Arc::INFO, "Client side MCCs are loaded");

  for(int n = 0;n<1;n++) {
  // Create and send echo request
  logger.msg(Arc::INFO, "Creating and sending request");
  Arc::NS echo_ns;
  Arc::PayloadSOAP req(echo_ns);
  // Making echo namespace appear at operation level only
  // This is probably not needed and is here for demonstration
  // purposes only.
  echo_ns["echo"]="urn:echo";
  Arc::XMLNode op = req.NewChild("echo:echo",echo_ns);
  op.NewChild("echo:say")="HELLO";
  Arc::PayloadSOAP* resp = NULL;
  Arc::MCC_Status status = client.process(&req,&resp);
  if(!status) {
    logger.msg(Arc::ERROR, "Request failed");
    if(resp) delete resp;
    return -1;
  };
  if(resp == NULL) {
    logger.msg(Arc::ERROR, "There is no response");
    return -1;
  };
  logger.msg(Arc::INFO, "Request succeed!!!");
  std::cout << "Response: " << (std::string)((*resp)["echoResponse"]["hear"])
	    << std::endl;
  delete resp;
  };
 
  return 0;
}
