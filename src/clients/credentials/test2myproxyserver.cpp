#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>
#include <fstream>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/message/MCCLoader.h>
#include <arc/client/ClientInterface.h>
#include <arc/credential/Credential.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "Test2MyProxyServer");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  std::string cert("../../tests/echo/usercert.pem");
  std::string key("../../tests/echo/userkey.pem");
  std::string cadir("../../tests/echo/certificates/");
  Arc::Credential signer(cert, key, cadir, "");

  //Generate a temporary self-signed proxy certificate
  //to contact the myproxy server
  std::string private_key, signing_cert, signing_cert_chain;
  std::string out_file("./tmpproxy.pem");

  Arc::Time t;
  int keybits = 1024;
  std::string req_str;
  Arc::Credential cred_request(t, Arc::Period(12*3600), keybits, "rfc","inheritAll", "", -1);
  cred_request.GenerateRequest(req_str);

  Arc::Credential proxy;
  proxy.InquireRequest(req_str);

  signer.SignRequest(&proxy, out_file.c_str());

  cred_request.OutputPrivatekey(private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);
  std::ofstream out_f(out_file.c_str(), std::ofstream::app);
  out_f.write(private_key.c_str(), private_key.size());
  out_f.write(signing_cert.c_str(), signing_cert.size());
  out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
  out_f.close();


  //Contact the myproxy server to delegate a certificate into that server

  // The message which will be sent to myproxy server
  std::string send_msg("VERSION=MYPROXYv2\n COMMAND=1\n USERNAME=mytest\n PASSPHRASE=123456\n LIFETIME=43200\n");
  send_msg.append("\0");

  std::cout<<"Send message to peer end through GSS communication: "<<send_msg<<" Size: "<<send_msg.length()<<std::endl;

  Arc::MCCConfig cfg;
  cfg.AddProxy(out_file);
  cfg.AddCADir("/home/qiangwz/.globus/certificates/");

  //Arc::ClientTCP client(cfg, "127.0.0.1", 7512, Arc::GSISec);
  Arc::ClientTCP client(cfg, "knowarc1.grid.niif.hu", 7512, Arc::GSISec);

  Arc::PayloadRaw request;
  request.Insert(send_msg.c_str(),0,send_msg.length());
  //Arc::PayloadRawInterface& buffer = dynamic_cast<Arc::PayloadRawInterface&>(request);
  //std::cout<<"Message in PayloadRaw:  "<<((Arc::PayloadRawInterface&)buffer).Content()<<std::endl;

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
    ret_str.append(ret_buf,len);
    memset(ret_buf,0,1024);
  }while(len == 1024);

  logger.msg(Arc::INFO, "Returned msg from myproxy server: %s ", ret_str.c_str());

  if(response) delete response;
  return 0;
}
