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
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  //std::string CAcert("./CAcert.pem"); 
  //std::string CAkey("./CAkey.pem");
  std::string CAserial("./CAserial");

  /**Generate CA certificate*/
  std::string ca_file("test_ca_cert.pem");
  std::string ca_key("test_ca_key.pem");
  std::string conf_file("test/ca.cnf");
  std::string ext_sect("v3_ca");

  int ca_keybits = 2048;
  Arc::Time ca_t;
  Arc::Credential ca(ca_t, Arc::Period(365*24*3600), ca_keybits, "EEC");

  BIO* ca_req_bio;
  ca_req_bio = BIO_new(BIO_s_mem());
  ca.GenerateRequest(ca_req_bio);

  std::string cert_dn("/O=TEST/CN=CA");
  std::string subkeyid("hash");
  ca.AddExtension("subjectKeyIdentifier", (char**)(subkeyid.c_str()));
  ca.AddExtension("authorityKeyIdentifier", (char**)("keyid:always,issuer"));
  //ca.AddExtension("basicConstraints", (char**)("CA:TRUE"));
  //ca.AddExtension("keyUsage",(char**)("nonRepudiation, digitalSignature, keyEncipherment"));
  ca.SelfSignEECRequest(cert_dn, conf_file.c_str(), ext_sect, ca_file.c_str());

  std::string ext_str = ca.GetExtension("basicConstraints");
  std::cout<<"basicConstraints of the ca cert: "<<ext_str<<std::endl;
 
  std::ofstream out_key(ca_key.c_str(), std::ofstream::out);
  std::string ca_private_key;
  ca.OutputPrivatekey(ca_private_key);
  out_key.write(ca_private_key.c_str(), ca_private_key.size());
  out_key.close();


  /**Generate certificate request on one side, 
  *and sign the certificate request on the other side.*/

  int keybits = 2048;
  Arc::Time t;

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
  Arc::Credential signer(ca_file, ca_key, CAserial, "", "", ca_passphrase);
  std::string identity_name = signer.GetIdentityName();
  std::cout<<"Identity Name: "<<identity_name<<std::endl;

  std::string dn("/O=KnowARC/OU=UIO/CN=Test001");
  signer.SignEECRequest(&eec, dn, out_certfile.c_str());

  ext_str = signer.GetExtension("basicConstraints");
  std::cout<<"basicConstraints of the ca cert: "<<ext_str<<std::endl;


  //Request side, output private key
  std::string private_key;
  std::string passphrase("12345");
  request.OutputPrivatekey(private_key, true, passphrase);
  std::ofstream out_f(out_keyfile.c_str());
  out_f.write(private_key.c_str(), private_key.size());
  out_f.close();

  return 0;
}
