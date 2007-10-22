#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/misc/ClientInterface.h>

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
  Arc::Loader service_loader(&service_config);
  logger.msg(Arc::INFO, "Service side MCCs are loaded");

  logger.msg(Arc::INFO, "Creating client interface");
  Arc::BaseConfig client_cfg(".");
  client_cfg.AddPluginsPath("../../hed/mcc/http/.libs");
  client_cfg.AddPluginsPath("../../hed/mcc/soap/.libs");
  client_cfg.AddPluginsPath("../../hed/mcc/tls/.libs");
  client_cfg.AddPluginsPath("../../hed/mcc/tcp/.libs");
  Arc::ClientSOAP client(client_cfg,"127.0.0.1",60000,true,"/echo1");
  logger.msg(Arc::INFO, "Client side MCCs are loaded");

  for(int n = 0;n<1;) {
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
