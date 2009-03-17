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
  //"PUT" command
  std::string send_msg("VERSION=MYPROXYv2\n COMMAND=1\n USERNAME=mytest\n PASSPHRASE=123456\n LIFETIME=43200\n");

  std::cout<<"Send message to peer end through GSS communication: "<<send_msg<<" Size: "<<send_msg.length()<<std::endl;

  Arc::MCCConfig cfg;
  cfg.AddProxy(out_file);
  cfg.AddCADir("$HOME/.globus/certificates/");

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
  char ret_buf[1024];
  memset(ret_buf,0,1024);
  int len;
  do {
    len = 1024;
    response->Get(&ret_buf[0], len);
    ret_str.append(ret_buf,len);
    memset(ret_buf,0,1024);
  }while(len == 1024);

  logger.msg(Arc::INFO, "Returned msg from myproxy server: %s   %d", ret_str.c_str(), ret_str.length());

  //Myproxy server will send back another message which includes 
  //the certificate request in DER format
  std::string x509ret_str;
  memset(ret_buf,0,1024);
  do {
    len = 1024;
    response->Get(&ret_buf[0], len);
    x509ret_str.append(ret_buf,len);
    memset(ret_buf,0,1024);
  }while(len == 1024);

  logger.msg(Arc::INFO, "Returned msg from myproxy server: %s   %d", x509ret_str.c_str(), x509ret_str.length());

  if(response) { delete response; response = NULL; }

  std::string tmp_req_file("myproxy_req.pem");
  std::ofstream tmp_req_out(tmp_req_file.c_str());
  tmp_req_out.write(x509ret_str.c_str(), x509ret_str.size());
  tmp_req_out.close();


  Arc::Credential signer1(out_file, "", cadir, "");

  Arc::Credential proxy1;
  std::string signedcert1, signing_cert1, signing_cert_chain1;
  proxy1.InquireRequest(x509ret_str,false,true);
  if(!(signer1.SignRequest(&proxy1, signedcert1, true))) {
    logger.msg(Arc::ERROR, "Delegate proxy failed");
    return 1;
  }

  signer1.OutputCertificate(signing_cert1, true);
  signer1.OutputCertificateChain(signing_cert_chain1, true);
  signedcert1.append(signing_cert1).append(signing_cert_chain1);
  //std::cout<<"Signing cert: "<<signing_cert1<<std::endl;
  //std::cout<<"Chain: "<<signing_cert_chain1<<std::endl;


  //logger.msg(Arc::INFO, "Generated proxy certificate: %s", signedcert1.c_str());

  std::string tmp_cert_file("myproxy_cert.pem");
  std::ofstream tmp_cert_out(tmp_cert_file.c_str());
  tmp_cert_out.write(signedcert1.c_str(), signedcert1.size());
  tmp_cert_out.close();


  //Send back the proxy certificate to myproxy server
  Arc::PayloadRaw request1;

  //Caculate the numbers of certifictes as the beginning of the message
  unsigned char number_of_certs;
  number_of_certs = 3;
  BIO* bio = BIO_new(BIO_s_mem());
  std::cout<<BIO_write(bio, &number_of_certs, sizeof(number_of_certs))<<std::endl;
  std::string start;
  for(;;) {
    char s[256];
    int l = BIO_read(bio,s,sizeof(s));
    if(l <= 0) break;
    start.append(s,l);
  }
  BIO_free_all(bio);
  signedcert1.insert(0,start);

  request1.Insert(signedcert1.c_str(),0,signedcert1.length());
  status = client.process(&request1, &response,true);
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

  std::string ret_str1;
  memset(ret_buf,0,1024);
  do {
    len = 1024;
    response->Get(&ret_buf[0], len);
    ret_str1.append(ret_buf,len);
    memset(ret_buf,0,1024);
  }while(len == 1024);
  logger.msg(Arc::INFO, "Returned msg from myproxy server: %s   %d", ret_str1.c_str(), ret_str1.length());


  if(response) { delete response; response = NULL; }

  return 0;
}
