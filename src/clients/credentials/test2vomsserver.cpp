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
  std::string send_msg("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms><command>G/playground.knowarc.eu</command><lifetime>43200</lifetime></voms>");

  std::cout<<"Send message to peer end through GSS communication: "<<send_msg<<" Size: "<<send_msg.length()<<std::endl;

  //std::string voms_url("httpg://arthur.hep.lu.se:15001");
  //Arc::URL voms_service(voms_url);

  Arc::MCCConfig cfg;
  cfg.AddProxy("/tmp/x509up_u1001");
  //cfg.AddCertificate("/home/wzqiang/arc-0.9/src/tests/echo/usercert.pem");
  //cfg.AddPrivateKey("/home/wzqiang/arc-0.9/src/tests/echo/userkey-nopass.pem");
  cfg.AddCADir("/home/qiangwz/.globus/certificates/");

  Arc::ClientTCP client(cfg, "arthur.hep.lu.se", 15002, Arc::GSISec);
  //Arc::ClientTCP client(cfg, "arthur.hep.lu.se", 15001, Arc::GSISec);
  //Arc::ClientTCP client(cfg, "squark.uio.no", 15011, Arc::GSISec);

  Arc::PayloadRaw request;
  request.Insert(send_msg.c_str(),0,send_msg.length());
  Arc::PayloadRawInterface& buffer = dynamic_cast<Arc::PayloadRawInterface&>(request);
  std::cout<<"Message in PayloadRaw:  "<<((Arc::PayloadRawInterface&)buffer).Content()<<std::endl;

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
 
  std::string ret_str;
  int length;
  char ret_buf[1024];
  memset(ret_buf,0,1024);
  int len;
  do {
    len = 1024;
    response->Get(&ret_buf[0], len);
    ret_str.append(ret_buf);
    memset(ret_buf,0,1024);
  }while(len == 1024);

  logger.msg(Arc::INFO, "Returned msg from voms server: %s ", ret_str.c_str());

  if(response) delete response;
  return 0;
}
