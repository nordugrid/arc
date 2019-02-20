#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <unistd.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>

#include <glibmm.h>

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);

  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  // Load service chain
  logger.msg(Arc::INFO, "Creating service side chain");
  Arc::Config service_config("service.xml");
  if(!service_config) {
    logger.msg(Arc::ERROR, "Failed to load service configuration");
    return -1;
  };
  Arc::MCCLoader service_loader(service_config);
  logger.msg(Arc::INFO, "Service side MCCs are loaded");
//  for(;;) sleep(10);
  logger.msg(Arc::INFO, "Creating client side chain");


  // Create client chain
  Arc::Config client_config("client.xml");
  if(!client_config) {
    logger.msg(Arc::ERROR, "Failed to load client configuration");
    return -1;
  };
  Arc::MCCLoader client_loader(client_config);
  logger.msg(Arc::INFO, "Client side MCCs are loaded");
  Arc::MCC* client_entry = client_loader["soap"];
  if(!client_entry) {
    logger.msg(Arc::ERROR, "Client chain does not have entry point");
    return -1;
  };

  Arc::MessageContext context;

  // -------------------------------------------------------
  //    Preparing delegation
  // -------------------------------------------------------
  std::string credentials;
  {
    std::ifstream ic("./cert.pem");
    for(;!ic.eof();) {
      char buf[256];
      ic.get(buf,sizeof(buf),0);
      if(ic.gcount() <= 0) break;
      credentials.append(buf,ic.gcount());
    };
  };
  {
    std::ifstream ic("key.pem");
    for(;!ic.eof();) {
      char buf[256];
      ic.get(buf,sizeof(buf),0);
      if(ic.gcount() <= 0) break;
      credentials.append(buf,ic.gcount());
    };
  };
  Arc::DelegationProviderSOAP deleg(credentials);
  if(!credentials.empty()) {
    logger.msg(Arc::INFO, "Initiating delegation procedure");
    if(!deleg.DelegateCredentialsInit(*client_entry,&context)) {
      logger.msg(Arc::ERROR, "Failed to initiate delegation");
      return -1;
    };
  };


  // -------------------------------------------------------
  //    Requesting information about service
  // -------------------------------------------------------
  {
  };

  for(int n = 0;n<1;n++) {


  };

  for(;;) sleep(10);

  return 0;
}
