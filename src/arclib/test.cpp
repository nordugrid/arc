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

  /*******************/
  //Use BIO as parameters 
  //Request side
  BIO* req;
  req = BIO_new(BIO_s_mem());
  ArcLib::Credential request(Arc::Time(), Arc::Period(12*3600));
  request.GenerateRequest(req);

  //Signing side
  BIO* out; 
  out = BIO_new(BIO_s_mem());
  ArcLib::Credential proxy(Arc::Time(), Arc::Period(12*3600));

  ArcLib::Credential signer(cert, key, "", cafile); 
  proxy.InquireRequest(req);
  signer.SignRequest(&proxy, out);

  BIO_free_all(req);
  BIO_free_all(out);


  /*******************/
  //Use string as parameters
  //Request side
  std::string req_string;
  std::string out_string;
  ArcLib::Credential request1(Arc::Time(), Arc::Period(12*3600));
  request1.GenerateRequest(req_string);
  std::cout<<"Certificate request: "<<req_string<<std::endl;

  //Signing side
  ArcLib::Credential proxy1(Arc::Time(), Arc::Period(12*3600));

  proxy1.InquireRequest(req_string);

  signer.SignRequest(&proxy1, out_string);
  std::cout<<"Signed proxy certificate: " <<out_string<<std::endl;


  /********************/
  //Use file location as parameters
  //Request side
  std::string req_file("./request.pem");
  std::string out_file("./out.pem");
  ArcLib::Credential request2(Arc::Time(), Arc::Period(12*3600));
  request2.GenerateRequest(req_file.c_str());

  //Signing side
  ArcLib::Credential proxy2(Arc::Time(), Arc::Period(12*3600));

  proxy2.InquireRequest(req_file.c_str());

  signer.SignRequest(&proxy2, out_file.c_str());

}
