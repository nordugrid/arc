#include <iostream>
#include <signal.h>

#include "../../libs/common/ArcConfig.h"
#include "../../libs/common/Logger.h"
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/message/SOAPEnvelope.h"
#include "../../hed/libs/message/PayloadSOAP.h"

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
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
  Arc::Loader service_loader(&service_config);
  logger.msg(Arc::INFO, "Service side MCCs are loaded");
  logger.msg(Arc::INFO, "Service is waiting for requests");
  
  for(;;) sleep(10);

  return 0;
}
