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
#include <arc/client/ClientInterface.h>

int main(void) {

  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  logger.msg(Arc::INFO, "Creating client side chain");

  std::string id;
  std::string url("https://localhost:60000/arex");
  Arc::NS ns;
  Arc::MCCConfig cfg;

  cfg.AddProxy("/home/roczei/.globus/proxy.pem");
  //cfg.AddCADir("/Users/roczei/arc1/etc/certificates");

  Arc::ClientSOAP client(cfg, url);


    std::string faultstring;

    Arc::PayloadSOAP request(ns);
    Arc::XMLNode req = request.NewChild("CacheCheck").NewChild("TheseFilesNeedToCheck");

    req.NewChild("FileURL") = "http://knowarc1.grid.niif.hu/storage/kiir";

    req.NewChild("FileURL") = "http://knowarc1.grid.niif.hu/storage/Makefile";

    req.NewChild("FileURL") = "http://knowarc1.grid.niif.hu/storage/rossz_file";


    Arc::PayloadSOAP* response;

    Arc::MCC_Status status = client.process(&request, &response);

    if (!status) {
           std::cerr << "Request failed" << std::endl;
    }

    std::string str;
    response->GetDoc(str, true);
    std::cout << str << std::endl;

   return 0;  
};

