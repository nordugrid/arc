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

  //
  // Create the AC on the VOMS side
  //

  std::string CAcert("ca_cert.pem");
  std::string voms_key_file("host_key.pem");
  std::string voms_cert_file("host_cert.pem");

  Arc::Credential voms_cred(voms_cert_file, voms_key_file, ".", CAcert);

  std::string user_proxy_file("user_proxy.pem");
  Arc::Credential proxy_cred(user_proxy_file, user_proxy_file, ".", CAcert);

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
  Arc::createVOMSAC(ac_str, voms_cred, proxy_cred, fqan, targets, attrs, voname, uri, 3600*12);
  
  //
  // Create the full AC which is an ordered list of AC 
  //

  ArcCredential::AC** aclist = NULL;
  std::string acorder;
  Arc::addVOMSAC(aclist, acorder, ac_str);

  std::string voms_proxy_file("voms_proxy.pem");

  int keybits = 1024;
  int proxydepth = 10;
  Arc::Time t;

  Arc::Credential proxy_req(t, Arc::Period(12*3600), keybits);
  std::string proxy_req_file("./request_withac.pem");
  proxy_req.GenerateRequest(proxy_req_file.c_str());

  //Signing side
  Arc::Credential proxy;
  proxy.InquireRequest(proxy_req_file.c_str());
  proxy.SetProxyPolicy("gsi2", "limited", "", proxydepth);
  //Add AC extension to proxy certificat before signing it
  proxy.AddExtension("acseq", (char**) aclist);

  // The User credentials should sign the voms proxy
  std::string user_cert_file("user_cert.pem");
  std::string user_key_file("user_key.pem");
  Arc::Credential user_cred(user_cert_file, user_key_file, ".", CAcert,"userpass");

  user_cred.SignRequest(&proxy, voms_proxy_file.c_str());

  std::string private_key, signing_cert, signing_cert_chain;
  proxy_req.OutputPrivatekey(private_key);
  user_cred.OutputCertificate(signing_cert);
  user_cred.OutputCertificateChain(signing_cert_chain);

  std::ofstream out_f(voms_proxy_file.c_str(), std::ofstream::app);
  out_f.write(private_key.c_str(), private_key.size());
  out_f.write(signing_cert.c_str(), signing_cert.size());
  out_f.write(signing_cert_chain.c_str(), signing_cert_chain.size());
  out_f.close();


  std::vector<std::string> vomscert_trust_dn;
  vomscert_trust_dn.push_back("/O=Grid/OU=ARC/CN=localhost");
  vomscert_trust_dn.push_back("^/O=Grid/O=NorduGrid");
  vomscert_trust_dn.push_back("NEXT CHAIN");
  vomscert_trust_dn.push_back("/O=Grid/OU=ARC/CN=CA");

  //
  // Read and pars VOMS proxy
  //

//  static Arc::Logger& logger = Arc::Logger::rootLogger;

  Arc::Credential voms_proxy(voms_proxy_file, "", ".", CAcert);

  std::vector<Arc::VOMSACInfo> attributes;
  Arc::parseVOMSAC(voms_proxy, ".", CAcert, vomscert_trust_dn, attributes, false); 
  
  for(size_t n=0; n<attributes.size(); n++) {
    for(size_t i=0; i<attributes[n].attributes.size(); i++) {
      Arc::CredentialLogger.msg(Arc::DEBUG, "Line %d.%d of the attributes returned: %s",n,i,attributes[n].attributes[i]);
    }
  }

  //CPPUNIT_ASSERT(attributes.size() == 1);
  //CPPUNIT_ASSERT(attributes[0].attributes.size() == 4);

}

CPPUNIT_TEST_SUITE_REGISTRATION(VOMSUtilTest);
