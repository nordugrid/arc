#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <arc/Logger.h>
#include "Credential.h"

int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  std::string CAcert("./CAcert.pem"); 
  std::string CAkey("./CAkey.pem");
  std::string CAserial("./CAserial");

  int keybits = 1024;
  Arc::Time t;

  /**Generate certificate request on one side, 
  *and sign the certificate request on the other side.*/

  //Request side
  std::string req_file("./eec_request.pem");
  std::string out_certfile("./eec_cert.pem");
  std::string out_keyfile("./eec_key.pem");
  ArcLib::Credential request(t, Arc::Period(168*3600), keybits, "EEC");
  request.GenerateRequest(req_file.c_str());

  //Signing side
  ArcLib::Credential eec;
  eec.InquireRequest(req_file.c_str(), true);
  //Add extension: a simple string
  std::string ext_data("test extension data");
  if(!(eec.AddExtension("1.2.3.4", ext_data))) {
    std::cout<<"Failed to add an extension to certificate"<<std::endl;
  }

  ArcLib::Credential signer(CAcert, CAkey, CAserial, 0, "", "");
  signer.SignEECRequest(&eec, out_certfile.c_str());

  //Request side, output private key
  std::string private_key;
  request.OutputPrivatekey(private_key);
  std::ofstream out_f(out_keyfile.c_str(), std::ofstream::app);
  out_f.write(private_key.c_str(), private_key.size());
  out_f.close();

  return 0;
}
