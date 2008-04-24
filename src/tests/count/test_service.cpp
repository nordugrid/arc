#include <iostream>
#include <signal.h>
#include <unistd.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

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
