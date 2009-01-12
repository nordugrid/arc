#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/client/ClientInterface.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "Test2VOMSServer");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  // The message which will be sent to voms server
  std::string send_msg("\
    <?xml version=\"1.0\" encoding = \"US-ASCII\"?>\
    <voms>\
     <command>knowarc.eu</command>\
     <base64>1</base64>\
     <version>4</version>\
     <lifetime>3600</lifetime>\
    </voms>");

  std::string voms_url("httpg://arthur.hep.lu.se:15001");
  Arc::URL voms_service(voms_url);

  Arc::MCCConfig cfg;
  cfg.AddProxy("/tmp/x509up_u1001");
  cfg.AddCADir("/home/qiangwz/.globus/certificates/");

  Arc::ClientTCP client(cfg, "arthur.hep.lu.se", 15001, Arc::GSISec);

  Arc::PayloadRaw request;
  request.Insert(send_msg.c_str(),0,send_msg.length());
  Arc::PayloadStreamInterface *response = NULL;
  Arc::MCC_Status status = client.process(&request, &response,true);
  if (!status) {
    logger.msg(Arc::ERROR, (std::string)status);
    if (response)
      delete response;
    return 1;
  }
  if (!response) {
    logger.msg(Arc::ERROR, "No stream response");
    return 1;
  }
  int length = 1024;
  char ret_buf[1024];
  memset(ret_buf,0,length);
  response->Get(ret_buf,length);
  std::string ret_str(ret_buf);
  logger.msg(Arc::INFO, "Returned msg from voms server: ", ret_str.c_str());

  if(response) delete response;
  return 0;
}
