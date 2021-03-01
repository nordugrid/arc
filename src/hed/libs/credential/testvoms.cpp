#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <arc/Logger.h>
#include "VOMSUtil.h"
#include "Credential.h"


int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);
  std::string cert("../../../tests/echo/testcert.pem");
  std::string key("../../../tests/echo/testkey-nopass.pem");
  std::string cafile("../../../tests/echo/testcacert.pem");
  std::string cadir("../../../tests/echo/certificates");

  int keybits = 2048;
  int proxydepth = 10;

  Arc::Time t;

  /**1.Create VOMS AC (attribute certificate), and put it into extension part of proxy certificate*/
  //Get information from a credential which acts as AC issuer.

  Arc::Credential issuer_cred(cert, key, cadir, cafile);

  //Get information from credential which acts as AC holder
  //Here we use the same credential for holder and issuer
  std::string cert1("./out.pem");
  std::string key1("./out.pem");
  Arc::Credential holder_cred(cert1, "", cadir, cafile);


  /** a.Below is voms-specific processing*/

  /* Prepare the AC and put it into string
  * in voms scenario, it is the voms server who prepare AC.
  * so voms server will then transfer the string into some Base64 (not sure) format, 
  * and then put the Base64 format into XML item
  * Here we just demostrate the process without using Base64 and XML encoding
  */

  std::vector<std::string> fqan;
  fqan.push_back("/knowarc.eu");

  std::vector<std::string> targets;
  targets.push_back("www.nordugrid.no");
 
  std::vector<std::string> attrs;
  attrs.push_back("::role=admin");
  attrs.push_back("::role=guest");

  std::string voname = "knowarc";
  std::string uri = "testvoms.knowarc.eu:50000";

  std::string ac_str;
  Arc::createVOMSAC(ac_str, issuer_cred, holder_cred, fqan, targets, attrs, voname, uri, 3600*12);
  //std::cout<<"AC: "<<ac_str<<std::endl;

  /* Parse the Attribute Certificate with string format
  * In real senario the Attribute Certificate with string format should be received from the other end, 
  * i.e. the voms-proxy-init(voms client) receives a few Attribute Certificates from different VOs
  * (voms server, and voms server signs AC), then composes the ACs into a AC list, and puts the AC 
  * list as a proxy certificate's extension.
  */
  ArcCredential::AC** aclist = NULL;
  std::string acorder;
  Arc::addVOMSAC(aclist, acorder, ac_str);
   

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
  //no, voms server can only generate proxy with "CN=proxy", AFAIK
  //The current voms code is not supposed to parsing proxy with "CN=336628850"?
  //Arc::Credential request(t, Arc::Period(12*3600), keybits, "gsi2", "limited", "", proxydepth);
  Arc::Credential request(t, Arc::Period(12*3600), keybits);
  request.GenerateRequest(req_file_ac.c_str());

  //Signing side
  Arc::Credential proxy;
  proxy.InquireRequest(req_file_ac.c_str());
  proxy.SetProxyPolicy("gsi2", "limited", "", proxydepth);
  //Add AC extension to proxy certificat before signing it
  proxy.AddExtension("acseq", (char**) aclist);

  //X509_EXTENSION* ext = NULL;
  //ext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("acseq"), (char*)aclist);

  std::string cert2("../../../tests/echo/testcert.pem");
  std::string key2("../../../tests/echo/testkey-nopass.pem");

  Arc::Credential signer(cert2, key2, cadir, cafile);
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
  // Consume the proxy certificate with AC extension
 
  std::string in_file_ac("./out_withac.pem");
  //std::string in_file_ac("./knowarc_voms.pem"); //Put here the proxy certificate generated from voms-proxy-init
  //std::string in_file_ac("./out.pem");
  std::string ca_cert_dir("../../../tests/echo/certificates");
  std::string ca_cert_file("../../../tests/echo/testcacert.pem");
  std::vector<std::string> vomscert_trust_dn;

  vomscert_trust_dn.push_back("^/O=Grid/O=NorduGrid");
  //vomscert_trust_dn.push_back("/O=Grid/O=NorduGrid/CN=NorduGrid ***");
  //vomscert_trust_dn.push_back("/O=Grid/O=NorduGrid/CN=NorduGrid abc");
  vomscert_trust_dn.push_back("NEXT CHAIN");
//  vomscert_trust_dn.push_back("/O=Grid/O=NorduGrid/OU=fys.uio.no/CN=Weizhong Qiang");
//  vomscert_trust_dn.push_back("/O=Grid/O=NorduGrid/CN=NorduGrid Certification Authority");
//  vomscert_trust_dn.push_back("NEXT CHAIN");
  vomscert_trust_dn.push_back("/O=Grid/O=Test/CN=localhost");
  vomscert_trust_dn.push_back("/O=Grid/O=Test/CN=CA");

  Arc::VOMSTrustList trust_dn(vomscert_trust_dn);
  Arc::Credential proxy2(in_file_ac, "", ca_cert_dir, ca_cert_file);
  std::vector<Arc::VOMSACInfo> attributes;
  Arc::parseVOMSAC(proxy2, ca_cert_dir, ca_cert_file, "", trust_dn, attributes); 

  for(size_t n=0; n<attributes.size(); n++) {
    for(size_t i=0; i<attributes[n].attributes.size(); i++) {
      std::cout<<"Line "<<n<<"."<<i<<" of the attributes returned: "<<attributes[n].attributes[i]<<std::endl;
    }
  }
  return 0;
}

