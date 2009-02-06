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
#include <arc/credential/VOMSAttribute.h>
#include <arc/credential/VOMSUtil.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "Test2VOMSServer");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  std::string cert("../../tests/echo/usercert.pem");
  std::string key("../../tests/echo/userkey.pem");
  std::string cadir("../../tests/echo/certificates/");
  Arc::Credential signer(cert, key, cadir, "");

  //Generate a temporary self-signed proxy certificate
  //to contact the voms server
  std::string private_key, signing_cert, signing_cert_chain;
  std::string out_file_ac("./out_withac.pem");

  Arc::Time t;
  int keybits = 1024;
  std::string req_str;
  Arc::Credential cred_request(t, Arc::Period(12*3600), keybits, "rfc","inheritAll", "", -1);
  cred_request.GenerateRequest(req_str);

  Arc::Credential proxy;
  proxy.InquireRequest(req_str);

  signer.SignRequest(&proxy, out_file_ac.c_str());

  cred_request.OutputPrivatekey(private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);
  std::ofstream out_f(out_file_ac.c_str(), std::ofstream::app);
  out_f.write(private_key.c_str(), private_key.size());
  out_f.write(signing_cert.c_str(), signing_cert.size());
  out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
  out_f.close();



  //Contact the voms server to retrieve attribute certificate

  // The message which will be sent to voms server
  //std::string send_msg("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms><command>G/playground.knowarc.eu</command><lifetime>43200</lifetime></voms>");
  std::string send_msg("<?xml version=\"1.0\" encoding = \"US-ASCII\"?><voms><command>G/knowarc.eu</command><lifetime>43200</lifetime></voms>");


  std::cout<<"Send message to peer end through GSS communication: "<<send_msg<<" Size: "<<send_msg.length()<<std::endl;

  Arc::MCCConfig cfg;
  cfg.AddProxy(out_file_ac);
  //cfg.AddProxy("/tmp/x509up_u1001");
  //cfg.AddCertificate("/home/wzqiang/arc-0.9/src/tests/echo/usercert.pem");
  //cfg.AddPrivateKey("/home/wzqiang/arc-0.9/src/tests/echo/userkey-nopass.pem");
  cfg.AddCADir("/home/qiangwz/.globus/certificates/");

  //Arc::ClientTCP client(cfg, "arthur.hep.lu.se", 15002, Arc::GSISec);
  Arc::ClientTCP client(cfg, "arthur.hep.lu.se", 15001, Arc::GSISec);
  //Arc::ClientTCP client(cfg, "squark.uio.no", 15011, Arc::GSISec);

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

  logger.msg(Arc::INFO, "Returned msg from voms server: %s ", ret_str.c_str());


  //Put the return attribute certificate into proxy certificate as the extension part
  Arc::XMLNode node(ret_str);
  std::string codedac;
  codedac = (std::string)(node["ac"]);
  std::cout<<"Coded AC: "<<codedac<<std::endl;
  std::string decodedac;
  int size;
  char* dec = NULL;
  dec = Arc::VOMSDecode((char *)(codedac.c_str()), codedac.length(), &size);
  decodedac.append(dec,size);
  if(dec!=NULL) { free(dec); dec = NULL; }
  //std::cout<<"Decoded AC: "<<decodedac<<std::endl<<" Size: "<<size<<std::endl;

  ArcCredential::AC** aclist = NULL;
  Arc::addVOMSAC(aclist, decodedac);

  Arc::Credential proxy1;
  proxy1.InquireRequest(req_str);
  //Add AC extension to proxy certificat before signing it
  proxy1.AddExtension("acseq", (char**) aclist);
  signer.SignRequest(&proxy1, out_file_ac.c_str());

  std::ofstream out_f1(out_file_ac.c_str(), std::ofstream::app);
  out_f1.write(private_key.c_str(), private_key.size());
  out_f1.write(signing_cert.c_str(), signing_cert.size());
  out_f1.write(signing_cert_chain.c_str(), signing_cert_chain.size());
  out_f1.close();


  if(response) delete response;
  return 0;
}
