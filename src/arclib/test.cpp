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

  Arc::Time t;

  /**Generate certificate request on one side, 
  *and sign the certificate request on the other side.*/
  /**1.Use BIO as parameters */

  //Request side
  BIO* req;
  req = BIO_new(BIO_s_mem());
  ArcLib::Credential request(t, Arc::Period(24*3600));//, 2048);// "rfc", "impersonation", "");
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
  ArcLib::Credential request1(t, Arc::Period(12*3600), 1024);
  request1.GenerateRequest(req_string);
  std::cout<<"Certificate request: "<<req_string<<std::endl;

  //Signing side
  ArcLib::Credential proxy1;
  proxy1.InquireRequest(req_string);

  signer.SignRequest(&proxy1, out_string);
  std::cout<<"Signed proxy certificate: " <<out_string<<std::endl;
  
  std::string signing_cert;
  signer.OutputCertificate(signing_cert);
  std::cout<<"Signing certificate: " <<signing_cert<<std::endl;

  //Back to request side, compose the signed proxy certificate, local private key, 
  //and signing certificate into one file.
  std::string private_key;
  request1.OutputPrivatekey(private_key);
  std::cout<<"Private key of request: " <<private_key<<std::endl;
  out_string.append(private_key);
  out_string.append(signing_cert);
  std::cout<<"Final proxy certificate: " <<out_string<<std::endl;


  /**3.Use file location as parameters*/
  //Request side
  std::string req_file("./request.pem");
  std::string out_file("./out.pem");
  ArcLib::Credential request2(t, Arc::Period(168*3600), 1024);
  request2.GenerateRequest(req_file.c_str());

  //Signing side
  ArcLib::Credential proxy2;
  proxy2.InquireRequest(req_file.c_str());

  signer.SignRequest(&proxy2, out_file.c_str());

  //Back to request side, compose the signed proxy certificate, local private key,
  //and signing certificate into one file.
  std::string private_key1;
  request1.OutputPrivatekey(private_key1);
  std::cout<<"Private key of request: " <<private_key<<std::endl;
  std::ofstream out_f(out_file.c_str(), std::ofstream::app);
  out_f.write(private_key1.c_str(), private_key1.size());
  out_f.write(signing_cert.c_str(), signing_cert.size());
  out_f.close();

  /**4.Create VOMS AC (attribute certificate), and put it into extension part of proxy certificate*/
  //Get information from a credential which acts as AC issuer.

  ArcLib::Credential issuer_cred(cert, key, "", cafile);

  //Get information from credential which acts as AC holder
  //Here we use the same credential for holder and issuer
  std::string cert1("./cert.pem");
  std::string key1("./key.pem");
  std::string cafile1("./ca.pem");   
  ArcLib::Credential holder_cred(cert1, key1,"", cafile1);


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
  std::string uri = "test.uio.no";

  std::string codedac;
  ArcLib::createVOMSAC(codedac, issuer_cred, holder_cred, fqan, targets, attrs, voname, uri, 3600*12);


  /* Parse the Attribute Certificate with string format
  * In real senario the Attribute Certificate with string format should be received from the other end, 
  * e.g. the voms-proxy-init(voms client) receives a few Attribute Certificates from different VOs(voms server, and voms server
  * signs AC), then composes the ACs into a AC list, and puts the AC list as a proxy certificate's extension.
  */
  AC** aclist = NULL;
  ArcLib::addVOMSAC(aclist, codedac);
   

  /** b.Below is general proxy processing, which is the same as the 
  *ordinary proxy certificate requesting and signing
  *except adding AC list as an extension to proxy certificate
  */
  //Use file location as parameters
  //Request side
  std::string req_file_ac("./request_withac.pem");
  std::string out_file_ac("./out_withac.pem");
  ArcLib::Credential request3(t, Arc::Period(12*3600), 2048);
  request3.GenerateRequest(req_file_ac.c_str());

  //Signing side
  ArcLib::Credential proxy3;
  proxy3.InquireRequest(req_file_ac.c_str());
  //Add AC extension to proxy certificat before signing it
  proxy3.AddExtension("acseq", (char**) aclist);


  //X509_EXTENSION* ext = NULL;
  //ext = X509V3_EXT_conf_nid(NULL, NULL, OBJ_txt2nid("acseq"), (char*)aclist);

  signer.SignRequest(&proxy3, out_file_ac.c_str());


  //TODO: Get the proxy certificate with voms AC extension, and parse the extension by using voms api.

}
