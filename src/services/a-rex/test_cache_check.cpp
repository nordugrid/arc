#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <map>
#include <iostream>
#include <signal.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/communication/ClientInterface.h>

int main(void) {

  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  logger.msg(Arc::INFO, "Creating client side chain");

  std::string id;
  std::string url("https://localhost/arex");
  Arc::NS ns("a-rex", "http://www.nordugrid.org/schemas/a-rex");
  Arc::MCCConfig cfg;
  Arc::UserConfig uc;
  uc.ApplyToConfig(cfg);

  Arc::ClientSOAP client(cfg, url, 60);
  std::string faultstring;

  Arc::PayloadSOAP request(ns);
  Arc::XMLNode req = request.NewChild("a-rex:CacheCheck").NewChild("a-rex:TheseFilesNeedToCheck");
  req.NewChild("a-rex:FileURL") = "http://example.org/test.txt";

  Arc::PayloadSOAP* response;

  Arc::MCC_Status status = client.process(&request, &response);

  if (!status) {
    std::cerr << "Request failed" << std::endl;
  }

  std::string str;
  response->GetDoc(str, true);
  std::cout << str << std::endl;

  return 0;
}

