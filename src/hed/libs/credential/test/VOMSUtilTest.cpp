#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <iostream>

#include <arc/Logger.h>
#include <arc/credential/VOMSUtil.h>

class VOMSUtilTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(VOMSUtilTest);
  CPPUNIT_TEST(VOMSTrustListTest);
  CPPUNIT_TEST_SUITE_END();

public:
  VOMSUtilTest() {}
  void setUp() {}
  void tearDown() {}
  void VOMSTrustListTest();
};

void VOMSUtilTest::VOMSTrustListTest() {

  std::vector<Arc::VOMSACInfo> output;
  std::string emptystring = "";
  Arc::VOMSTrustList emptylist;

  //  CPPUNIT_ASSERT_EQUAL(!parseVOMSAC(c, emptystring, emptystring, emptylist, output, false),false);

  // Create the AC on the VOMS side

  std::string CAcert("ca_cert.pem");
  std::string user_key_file("user_key.pem");
  std::string user_cert_file("user_cert.pem");
  Arc::Credential issuer_cred(user_cert_file, user_key_file, "", CAcert, false, "userpassword");

  std::string vomsserver_key_file("host_key.pem");
  std::string vomsserver_cert_file("host_cert.pem");
  Arc::Credential ac_issuer_cred(vomsserver_cert_file, vomsserver_key_file, "", CAcert, false, "");

  std::string holder_proxy_file("user_proxy.pem");
  Arc::Credential holder_cred(holder_proxy_file, "", "", CAcert, false);

  std::vector<std::string> fqan;
  fqan.push_back("/nordugrid.org");

  std::vector<std::string> targets;
  targets.push_back("www.nordugrid.org");
 
  std::vector<std::string> attrs;
  attrs.push_back("::role=admin");
  attrs.push_back("::role=guest");

  std::string voname = "nordugrid";
  std::string uri = "voms.nordugrid.org:50000";

  std::string ac_str;
  Arc::createVOMSAC(ac_str, ac_issuer_cred, holder_cred, fqan, targets, attrs, voname, uri, 3600*12);
 
 
  //
  // Create the full AC which is an ordered list of AC 
  //

  // encode the AC string into base64
  int size;
  char* enc = NULL;
  std::string ac_str_b64;
  enc = Arc::VOMSEncode((char*)(ac_str.c_str()), ac_str.length(), &size);
  if (enc != NULL) {
    ac_str_b64.append(enc, size);
    free(enc);
    enc = NULL;
  }
  std::string aclist_str;
  aclist_str.append(VOMS_AC_HEADER).append("\n");
  aclist_str.append(ac_str_b64).append("\n");
  aclist_str.append(VOMS_AC_TRAILER).append("\n");

  std::string voms_proxy_file("voms_proxy.pem");

  // Request
  int keybits = 2048;
  int proxydepth = 10;
  Arc::Time t;
  Arc::Credential proxy_req(t, Arc::Period(12*3600), keybits, "gsi2", "limited", "", proxydepth);
  std::string proxy_req_file("voms_req.pem");
  proxy_req.GenerateRequest(proxy_req_file.c_str());

  //Add AC extension to proxy certificat before signing it
  proxy_req.AddExtension("acseq", (char**) (aclist_str.c_str()));

  //Sign the voms proxy
  issuer_cred.SignRequest(&proxy_req, voms_proxy_file.c_str());

  std::string private_key, signing_cert, signing_cert_chain;
  proxy_req.OutputPrivatekey(private_key);
  issuer_cred.OutputCertificate(signing_cert);
  issuer_cred.OutputCertificateChain(signing_cert_chain);

  std::ofstream out_f(voms_proxy_file.c_str(), std::ofstream::app);
  out_f.write(private_key.c_str(), private_key.size());
  out_f.write(signing_cert.c_str(), signing_cert.size());
  out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
  out_f.close();


  std::vector<std::string> vomscert_trust_dn;
  vomscert_trust_dn.push_back("/O=Grid/OU=ARC/CN=localhost");
  vomscert_trust_dn.push_back("/O=Grid/OU=ARC/CN=CA");
  vomscert_trust_dn.push_back("NEXT CHAIN");
  vomscert_trust_dn.push_back("^/O=Grid/OU=ARC");

  // Read and pars VOMS proxy

  Arc::Credential voms_proxy(voms_proxy_file, "", ".", CAcert, false);

  std::vector<Arc::VOMSACInfo> attributes;
  Arc::VOMSTrustList trust_dn(vomscert_trust_dn);
  Arc::parseVOMSAC(voms_proxy, ".", CAcert, "", trust_dn, attributes, true); 
  
  for(size_t n=0; n<attributes.size(); n++) {
    for(size_t i=0; i<attributes[n].attributes.size(); i++) {
      Arc::CredentialLogger.msg(Arc::DEBUG, "Line %d.%d of the attributes returned: %s",n,i,attributes[n].attributes[i]);
    }
  }

  CPPUNIT_ASSERT_EQUAL(1,(int)attributes.size());
  CPPUNIT_ASSERT_EQUAL(4,(int)attributes[0].attributes.size());

}

CPPUNIT_TEST_SUITE_REGISTRATION(VOMSUtilTest);
