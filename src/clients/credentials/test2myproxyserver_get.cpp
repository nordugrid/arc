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
  //"GET" command
  std::string send_msg("VERSION=MYPROXYv2\n COMMAND=0\n USERNAME=mytest\n PASSPHRASE=123456\n LIFETIME=43200\n");

  std::cout<<"Send message to peer end through GSS communication: "<<send_msg<<" Size: "<<send_msg.length()<<std::endl;

  //For "GET" command, client authentication is optional
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

  if(response) { delete response; response = NULL; }


  //Generate a certificate request,
  //and send it to myproxy server
  std::string x509_req_str;
  Arc::Time start;
  Arc::Credential x509_request(start, Arc::Period(), 1024);
  x509_request.GenerateRequest(x509_req_str, true);
  std::string proxy_key_str;
  x509_request.OutputPrivatekey(proxy_key_str);

  Arc::PayloadRaw request1;
  request1.Insert(x509_req_str.c_str(),0,x509_req_str.length());
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

  BIO* bio = BIO_new(BIO_s_mem());
  BIO_write(bio,(unsigned char *)(ret_str1.c_str()),ret_str1.length());
  unsigned char number_of_certs;
  BIO_read(bio, &number_of_certs, sizeof(number_of_certs));
  logger.msg(Arc::INFO, "There are %d certificates in the returned msg",number_of_certs);
  std::string proxy_cert_str;
  for(;;) {
    char s[256];
    int l = BIO_read(bio,s,sizeof(s));
    if(l <= 0) break;
    proxy_cert_str.append(s,l);
  }
  BIO_free_all(bio);

  //Output the PEM formated proxy certificate
  std::string tmpcert_file("tmpcert.pem");
  std::ofstream tmpcert_f(tmpcert_file.c_str());
  std::string tmpkey_file("tmpkey.pem");
  std::ofstream tmpkey_f(tmpkey_file.c_str());
  tmpcert_f.write(proxy_cert_str.c_str(), proxy_cert_str.size());
  tmpkey_f.write(proxy_key_str.c_str(), proxy_key_str.size());
  tmpcert_f.close();
  tmpkey_f.close();
  
  Arc::Credential proxy_cred(tmpcert_file,tmpkey_file, cadir, "");
  std::string proxy_cred_str_pem;
  std::string proxy_cred_file("proxy_cred.pem");
  std::ofstream proxy_cred_f(proxy_cred_file.c_str());
  proxy_cred.OutputCertificate(proxy_cred_str_pem);
  proxy_cred.OutputPrivatekey(proxy_cred_str_pem);
  proxy_cred.OutputCertificateChain(proxy_cred_str_pem);
  proxy_cred_f.write(proxy_cred_str_pem.c_str(), proxy_cred_str_pem.size());
  proxy_cred_f.close();


  //Myproxy server will then return a standard response message
  std::string ret_str2;
  memset(ret_buf,0,1024);
  do {
    len = 1024;
    response->Get(&ret_buf[0], len);
    ret_str2.append(ret_buf,len);
    memset(ret_buf,0,1024);
  }while(len == 1024);
  logger.msg(Arc::INFO, "Returned msg from myproxy server: %s   %d", ret_str2.c_str(), ret_str2.length());

  if(response) { delete response; response = NULL; }

  return 0;
}
