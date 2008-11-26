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
  Arc::Credential request(t, Arc::Period(168*3600), keybits, "EEC");
  request.GenerateRequest(req_file.c_str());

  //Signing side
  Arc::Credential eec;
  eec.InquireRequest(req_file.c_str(), true);
  //Add extension: a simple string
  std::string ext_data("test extension data");
  if(!(eec.AddExtension("1.2.3.4", ext_data))) {
    std::cout<<"Failed to add an extension to certificate"<<std::endl;
  }
  
  std::string ca_passphrase = "aa1122";
  Arc::Credential signer(CAcert, CAkey, CAserial, 0, "", "", ca_passphrase);
  std::string dn("/O=KnowARC/OU=UIO/CN=Test001");
  signer.SignEECRequest(&eec, dn, out_certfile.c_str());

  //Request side, output private key
  std::string private_key;
  std::string passphrase("12345");
  request.OutputPrivatekey(private_key, true, passphrase);
  std::ofstream out_f(out_keyfile.c_str());
  out_f.write(private_key.c_str(), private_key.size());
  out_f.close();

  return 0;
}
