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

  std::string cert("./usercert.pem"); 
  std::string key("./userkey-nopass.pem");
  std::string cadir("./testca"); 

  int keybits = 1024;
  int proxydepth = 10;

  Arc::Time t;

  /**1.Create VOMS AC (attribute certificate), and put it into extension part of proxy certificate*/
  //Get information from a credential which acts as AC issuer.

  ArcLib::Credential issuer_cred(cert, key, cadir, "");

  //Get information from credential which acts as AC holder
  //Here we use the same credential for holder and issuer
  std::string cert1("./out.pem");
  std::string key1("./out.pem");
  ArcLib::Credential holder_cred(cert1, key1, cadir, "");


  /** a.Below is voms-specific processing*/

  /* Prepare the AC and put it into string
  * in voms scenario, it is the voms server who prepare AC.
  * so voms server will then transfer the string into some Base64 (not sure) format, 
  * and then put the Base64 format into XML item
  * Here we just demostrate the process without using Base64 and XML encoding
  */

  std::vector<std::string> fqan;
  fqan.push_back("knowarc.eu");

  std::vector<std::string> targets;
  targets.push_back("www.nordugrid.no");
 
  std::vector<std::string> attrs;
  attrs.push_back("::role=admin");
  attrs.push_back("::role=guest");

  std::string voname = "knowarc";
  std::string uri = "testvoms.knowarc.eu:50000";

  std::string codedac;
  ArcLib::createVOMSAC(codedac, issuer_cred, holder_cred, fqan, targets, attrs, voname, uri, 3600*12);


  /* Parse the Attribute Certificate with string format
  * In real senario the Attribute Certificate with string format should be received from the other end, 
  * i.e. the voms-proxy-init(voms client) receives a few Attribute Certificates from different VOs
  * (voms server, and voms server signs AC), then composes the ACs into a AC list, and puts the AC 
  * list as a proxy certificate's extension.
  */
  ArcLib::AC** aclist = NULL;
  ArcLib::addVOMSAC(aclist, codedac);
   

  /** b.Below is general proxy processing, which is the same as the 
  *ordinary proxy certificate requesting and signing
  *except adding AC list as an extension to proxy certificate
  *In the case of voms proxy certificate generation, since voms proxy is a self-signed 
  *proxy, both the request and signing side are the client itself.
  */
  //Use file location as parameters
  //Request side
  std::string req_file_ac("./request_withac.pem");
  std::string out_file_ac("./out_withac.pem");

  //The voms server is not supposed to generate rfc proxy?
  //The current voms code is not supposed to parsing proxy with "CN=336628850"?
  ArcLib::Credential request(t, Arc::Period(12*3600), keybits, "gsi2", "limited", "", proxydepth);
  request.GenerateRequest(req_file_ac.c_str());

  //Signing side
  ArcLib::Credential proxy;
  proxy.InquireRequest(req_file_ac.c_str());
  //Add AC extension to proxy certificat before signing it
  proxy.AddExtension("acseq", (char**) aclist);

  //X509_EXTENSION* ext = NULL;
  //ext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("acseq"), (char*)aclist);

  std::string cert2("./cert.pem");
  std::string key2("./key.pem");
  ArcLib::Credential signer(cert2, key2, cadir, "");
  signer.SignRequest(&proxy, out_file_ac.c_str());

  //Back to request side, compose the signed proxy certificate, local private key,
  //and signing certificate into one file.
  std::string private_key, signing_cert, signing_cert_chain;
  request.OutputPrivatekey(private_key);
  signer.OutputCertificate(signing_cert);
  signer.OutputCertificateChain(signing_cert_chain);
  std::ofstream out_f(out_file_ac.c_str(), std::ofstream::app);
  out_f.write(private_key.c_str(), private_key.size());
  out_f.write(signing_cert.c_str(), signing_cert.size());
  out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
  out_f.close();


  /*2. Get the proxy certificate with voms AC extension, and parse the extension.*/
  //Use file location as parameters
  // Consume the proxy certificate with AC extenstion
 
  std::string in_file_ac("./out_withac.pem");
  //std::string in_file_ac("./knowarc_voms.pem");
  std::string ca_cert_dir("./testca");
  std::string vomsdir(".");
  ArcLib::Credential proxy2(in_file_ac, in_file_ac, ca_cert_dir, "");
  std::vector<std::string> attributes;
  ArcLib::parseVOMSAC(proxy2, ca_cert_dir, vomsdir, attributes); 

  int i;
  for(i=0; i<attributes.size(); i++) {
    std::cout<<"Line "<<i<<" of the attributes returned: "<<attributes[i]<<std::endl;
  }
  return 0;
}
