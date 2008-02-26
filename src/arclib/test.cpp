#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <arc/Logger.h>
#include "Credential.h"
#include "voms_util.h"
extern "C" {
#include "listfunc.h"
}


int main(void) {
  Arc::LogStream cdest(std::cerr);
  Arc::Logger::getRootLogger().addDestination(cdest);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  std::string cert("./cert.pem"); 
  std::string key("./key.pem");
  std::string cafile("./ca.pem"); 

  /**Generate certificate request on one side, 
  *and sign the certificate request on the other side.*/
  /**1.Use BIO as parameters */

  //Request side
  BIO* req;
  req = BIO_new(BIO_s_mem());
  ArcLib::Credential request(Arc::Time(), Arc::Period(24*3600), 2048);
  request.GenerateRequest(req);

  //Signing side
  BIO* out; 
  out = BIO_new(BIO_s_mem());
  ArcLib::Credential proxy;

  ArcLib::Credential signer(cert, key, "", cafile); 
  proxy.InquireRequest(req);
  signer.SignRequest(&proxy, out);

  BIO_free_all(req);
  BIO_free_all(out);



  /**2.Use string as parameters */
  //Request side
  std::string req_string;
  std::string out_string;
  ArcLib::Credential request1(Arc::Time(), Arc::Period(12*3600), 1024);
  request1.GenerateRequest(req_string);
  std::cout<<"Certificate request: "<<req_string<<std::endl;

  //Signing side
  ArcLib::Credential proxy1;

  proxy1.InquireRequest(req_string);

  signer.SignRequest(&proxy1, out_string);
  std::cout<<"Signed proxy certificate: " <<out_string<<std::endl;



  /**3.Use file location as parameters*/
  //Request side
  std::string req_file("./request.pem");
  std::string out_file("./out.pem");
  ArcLib::Credential request2(Arc::Time(), Arc::Period(168*3600), 1024);
  request2.GenerateRequest(req_file.c_str());

  //Signing side
  ArcLib::Credential proxy2;

  proxy2.InquireRequest(req_file.c_str());

  signer.SignRequest(&proxy2, out_file.c_str());



  /**4.Create VOMS AC (attribute certificate), and put it into extension part of proxy certificate*/
  //Get information from a credential which acts as AC issuer.
  EVP_PKEY* issuerkey = NULL;
  X509* holder = NULL;
  X509* issuer = NULL;
  AC* ac = NULL; 
  STACK_OF(X509)* issuerchain = NULL;

  ArcLib::Credential issuer_cred(cert, key, "", cafile);
  issuer = issuer_cred.GetCert();
  issuerchain = issuer_cred.GetCertChain();
  issuerkey = issuer_cred.GetPrivKey();

  //Get information from credential which acts as AC holder
  //Here we use the same credential for holder and issuer
  std::string cert1("./cert.pem");
  std::string key1("./key.pem");
  std::string cafile1("./ca.pem");   
  ArcLib::Credential holder_cred(cert1, key1,"", cafile1);
  holder = holder_cred.GetCert();


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

  ac = AC_new();

  ArcLib::createVOMSAC(issuer, issuerchain, holder, issuerkey, (BIGNUM *)(BN_value_one()),
             fqan, targets, attrs, &ac, "knowarc", "test.uio.no", 3600*12);

  unsigned int len = i2d_AC(ac, NULL);
  unsigned char *tmp = (unsigned char *)OPENSSL_malloc(len);
  unsigned char *ttmp = tmp;
  std::string codedac;
  if (tmp) {
    i2d_AC(ac, &tmp);
    codedac = std::string((char *)ttmp, len);
  }
  free(ttmp);  

  AC_free(ac);


  /* Parse the Attribute Certificate with string format
  * In real senario the Attribute Certificate with string format should be received from the other end.
  */
  AC* received_ac;
  AC** aclist = NULL;
  AC** actmplist = NULL;
  char *p, *pp;
  int l = codedac.size();

  pp = (char *)malloc(codedac.size());
  if(!pp) {std::cerr<<"Can not allocate memory for parsing ac"<<std::endl; return (0); }

  pp = (char *)memcpy(pp, codedac.data(), codedac.size());
  p = pp;

  //Parse the AC, and insert it into an AC array
  if((received_ac = d2i_AC(NULL, (unsigned char **)&p, l))) {
    actmplist = (AC **)listadd((char **)aclist, (char *)received_ac, sizeof(AC *));
    if (actmplist) {
      aclist = actmplist;
    }
    else {
      listfree((char **)aclist, (freefn)AC_free);
    }
  }
  free(pp);
   

  /** b.Below is general proxy processing, which is the same as the 
  *ordinary proxy certificate requesting and signing
  *except adding AC list as an extension to proxy certificate
  */
  //Use file location as parameters
  //Request side
  std::string req_file_ac("./request_withac.pem");
  std::string out_file_ac("./out_withac.pem");
  ArcLib::Credential request3(Arc::Time(), Arc::Period(12*3600), 2048);
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
