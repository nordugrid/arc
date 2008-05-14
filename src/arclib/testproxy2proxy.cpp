#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <arc/Logger.h>
#include "Credential.h"
#include "voms_util.h"


int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  std::string cafile("./ca.pem");

  /**Generate a proxy certificate based on existing proxy certificate*/
  //Request side
  std::string req_file("./request1.pem");
  std::string out_file("./out1.pem");
  ArcLib::Credential request(Arc::Time(), Arc::Period(168*3600), 1024);
  request.GenerateRequest(req_file.c_str());

  //Signing side
  ArcLib::Credential proxy;

  std::string cert_proxy("./out.pem");
  ArcLib::Credential signer(cert_proxy, "", "", cafile);

  proxy.InquireRequest(req_file.c_str());

  signer.SignRequest(&proxy, out_file.c_str());

}
