#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <arc/Logger.h>
#include "Credential.h"
#include "voms_util.h"


int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  std::string cert("./cert.pem"); 
  std::string key("./key.pem");
  std::string cafile("./ca.pem"); 

  int keybits = 1024;
  int proxydepth = 10;

  Arc::Time t;

  /**Generate certificate request on one side, 
  *and sign the certificate request on the other side.*/
  /**1.Use BIO as parameters */

  //Request side
  BIO* req;
  req = BIO_new(BIO_s_mem());
  ArcLib::Credential request(t, Arc::Period(24*3600), keybits,  "rfc", "independent");
  request.GenerateRequest(req);

  //Signing side
  BIO* out; 
  out = BIO_new(BIO_s_mem());
  ArcLib::Credential proxy;

  ArcLib::Credential signer(cert, key, "", cafile); 
  std::string dn_name = signer.GetDN();
  std::cout<<"DN:--"<<dn_name<<std::endl;

  proxy.InquireRequest(req);
  signer.SignRequest(&proxy, out);

  BIO_free_all(req);
  BIO_free_all(out);


  /**2.Use string as parameters */
  //Request side
  std::string req_string;
  std::string out_string;
  ArcLib::Credential request1(t, Arc::Period(12*3600));//, keybits, "rfc", "independent");
  request1.GenerateRequest(req_string);
  std::cout<<"Certificate request: "<<req_string<<std::endl;

  //Signing side
  ArcLib::Credential proxy1;
  proxy1.InquireRequest(req_string);

  signer.SignRequest(&proxy1, out_string);
  
  std::string signing_cert1;
  signer.OutputCertificate(signing_cert1);

  //Back to request side, compose the signed proxy certificate, local private key, 
  //and signing certificate into one file.
  std::string private_key1;
  request1.OutputPrivatekey(private_key1);
  out_string.append(private_key1);
  out_string.append(signing_cert1);
  std::cout<<"Final proxy certificate: " <<out_string<<std::endl;


  /**3.Use file location as parameters*/
  //Request side
  std::string req_file("./request.pem");
  std::string out_file("./out.pem");
  ArcLib::Credential request2(t, Arc::Period(168*3600), keybits, "rfc", "inheritall", "policy.txt", proxydepth);
  request2.GenerateRequest(req_file.c_str());

  //Signing side
  ArcLib::Credential proxy2;
  proxy2.InquireRequest(req_file.c_str());

  signer.SignRequest(&proxy2, out_file.c_str());

  //Back to request side, compose the signed proxy certificate, local private key,
  //and signing certificate into one file.
  std::string private_key2, signing_cert2, signing_cert2_chain;
  request2.OutputPrivatekey(private_key2);
  signer.OutputCertificate(signing_cert2);
  signer.OutputCertificateChain(signing_cert2_chain);
  std::ofstream out_f(out_file.c_str(), std::ofstream::app);
  out_f.write(private_key2.c_str(), private_key2.size());
  out_f.write(signing_cert2.c_str(), signing_cert2.size());
  out_f.write(signing_cert2_chain.c_str(), signing_cert2_chain.size());
  out_f.close();

  return 0;
}
