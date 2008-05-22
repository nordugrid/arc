#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <arc/Logger.h>
#include "Credential.h"

#include <openssl/x509.h>

int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  std::string cafile("./ca.pem");

  int keybits = 1024;
  int proxydepth = 10;

  /**Generate a proxy certificate based on existing proxy certificate*/
  //Request side
  std::string req_file1("./request1.pem");
  std::string out_file1("./proxy1.pem");
  ArcLib::Credential request1(Arc::Time(), Arc::Period(168*3600), keybits, "rfc", "independent", "", proxydepth);
  request1.GenerateRequest(req_file1.c_str());

  //Signing side
  ArcLib::Credential proxy1;
  std::string signer_cert1("./out.pem");
  ArcLib::Credential signer1(signer_cert1, "", "", cafile);
  proxy1.InquireRequest(req_file1.c_str());
  signer1.SignRequest(&proxy1, out_file1.c_str());

  //Back to request side, compose the signed proxy certificate, local private key,
  //and signing certificate into one file.
  std::string private_key1, signing_cert1, signing_cert1_chain;
  request1.OutputPrivatekey(private_key1);
  signer1.OutputCertificate(signing_cert1);
  signer1.OutputCertificateChain(signing_cert1_chain);
  std::ofstream out_f1(out_file1.c_str(), std::ofstream::app);
  out_f1.write(private_key1.c_str(), private_key1.size());
  out_f1.write(signing_cert1.c_str(), signing_cert1.size());
  out_f1.write(signing_cert1_chain.c_str(), signing_cert1_chain.size());
  out_f1.close();

  //Generate one more proxy based on the proxy which just has been generated
  std::string req_file2("./request2.pem");
  std::string out_file2("./proxy2.pem");
  ArcLib::Credential request2(Arc::Time(), Arc::Period(168*3600), keybits,  "rfc", "independent", "", proxydepth);
  request2.GenerateRequest(req_file2.c_str());

  //Signing side
  ArcLib::Credential proxy2;
  std::string signer_cert2("./proxy1.pem");
  ArcLib::Credential signer2(signer_cert2, "", "", cafile);
  proxy2.InquireRequest(req_file2.c_str());
  signer2.SignRequest(&proxy2, out_file2.c_str());

  //Back to request side, compose the signed proxy certificate, local private key,
  //and signing certificate into one file.
  std::string private_key2, signing_cert2, signing_cert2_chain;
  request2.OutputPrivatekey(private_key2);
  signer2.OutputCertificate(signing_cert2);
  signer2.OutputCertificateChain(signing_cert2_chain);
  std::ofstream out_f2(out_file2.c_str(), std::ofstream::app);
  out_f2.write(private_key2.c_str(), private_key2.size());
  out_f2.write(signing_cert2.c_str(), signing_cert2.size());
  //Here add the up-level signer certificate
  out_f2.write(signing_cert2_chain.c_str(), signing_cert2_chain.size());
  out_f2.close();

  /*****************************************/
  std::string cert_file("./proxy2.pem"), issuer_file("./proxy1.pem");

  BIO *cert_in=NULL;
  BIO *issuer_in=NULL;
  X509 *cert = NULL;
  X509 *issuer = NULL;
  issuer_in=BIO_new_file(issuer_file.c_str(), "r"); 
  PEM_read_bio_X509(issuer_in,&issuer,NULL,NULL); 

  cert_in=BIO_new_file(cert_file.c_str(), "r");
  PEM_read_bio_X509(cert_in,&cert,NULL,NULL);

  EVP_PKEY *pkey=NULL;
  pkey = X509_get_pubkey(issuer);
  int n = X509_verify(cert,pkey);

  BIO *pubkey;
  std::string pubkey_file("pubkey1.pem");
  pubkey = BIO_new_file(pubkey_file.c_str(), "w");
  PEM_write_bio_PUBKEY(pubkey, pkey);

  std::cout<<"Verification result"<<n<<std::endl;

}
