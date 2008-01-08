#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <arc/Logger.h>
#include "Credential.h"

int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  std::string cert("./cert.pem"); 
  std::string key("./key.pem");
  std::string cafile("./ca.pem"); 
 
  //Request side
  BIO* req;
  req = BIO_new(BIO_s_mem());
  ArcLib::Credential request;
  request.GenerateRequest(req);

  //Signing side
  BIO* out; 
  out = BIO_new(BIO_s_mem());
  ArcLib::Credential proxy;

  ArcLib::Credential signer(cert, key, "", cafile); 
  proxy.InquireRequest(req);
  signer.SignRequest(&proxy, out);

}
