#include <cppunit/extensions/HelperMacros.h>

#include <iostream>

#include <openssl/evp.h>
#include <arc/credential/CertUtil.h>
#include <arc/credential/Credential.h>

class CredentialTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(CredentialTest);
  CPPUNIT_TEST(CertTest);
  CPPUNIT_TEST_SUITE_END();

public:
  CredentialTest() {}
  void setUp() {}
  void tearDown() {}
  void CertTest();
};

void CredentialTest::CertTest() {

  // TODO: The CA cert should really be generated with the Credential Class 
  std::string CAdn("/O=Grid/OU=ARC/CN=ARC CA");
  std::string CAcert("ca_cert.pem");
  std::string CAkey("ca_key.pem");
  std::string CAserial("ca_serial");
  std::string CApassphrase("password");

  std::ofstream out_f;
  // Create serial file
  out_f.open(CAserial.c_str());
  out_f << "00";
  out_f.close();

  Arc::Credential ca(CAcert, CAkey, CAserial, 0, "", "", CApassphrase);

  // Did we load a CA cert?
  CPPUNIT_ASSERT_EQUAL(ca.GetType(),ArcCredential::CERT_TYPE_CA);

  // Test if the DN is read properly
  CPPUNIT_ASSERT_EQUAL(CAdn,ca.GetDN());


  // Default EEC values
  int keybits = 1024;
  Arc::Time t;

  // Hostcert signing

  std::string host_req_file("host_req.pem");
  std::string host_key_file("host_key.pem");
  std::string host_cert_file("host_cert.pem");

  Arc::Credential host_req(t, Arc::Period(365*24*3600), keybits, "EEC");
  host_req.GenerateRequest(host_req_file.c_str());

  // Write private key to file for loading later - no passphrase for hosts
  std::string host_key;
  host_req.OutputPrivatekey(host_key);
  out_f.open(host_key_file.c_str());
  out_f << host_key;;
  out_f.close();

  // Load the request
  Arc::Credential host_eec;
  host_eec.InquireRequest(host_req_file.c_str(), true);
  host_eec.SetLifeTime(Arc::Period(365*24*60*60));

  // Add subjectAltname extension to host cert
  std::string host_ext("DNS:localhost");
  host_eec.AddExtension("2.5.29.17", host_ext);

  // Sign request
  std::string host_dn("/O=Grid/OU=ARC/CN=localhost");
  ca.SignEECRequest(&host_eec, host_dn, host_cert_file.c_str());

  //Load signed host cert
  Arc::Credential host_cert(host_cert_file,host_key_file,".",CAcert);
  
  //Does the certificate chain verify?
  CPPUNIT_ASSERT(host_cert.GetVerification());
  // Did the signed cert get the right DN?
  CPPUNIT_ASSERT_EQUAL(host_dn,host_cert.GetDN());
  // Right type?
  CPPUNIT_ASSERT_EQUAL(host_cert.GetType(),ArcCredential::CERT_TYPE_EEC);


  //
  // Now User signing
  //

  std::string user_req_file("user_req.pem");
  std::string user_key_file("user_key.pem");
  std::string user_cert_file("user_cert.pem");

  Arc::Period user_life(365*24*60*60);
  Arc::Credential user_req(t, Arc::Period(365*24*3600), keybits, "EEC");
  user_req.GenerateRequest(user_req_file.c_str());

  // Write private key to file for loading later
  std::string user_key;
  std::string user_pass("userpass");
  user_req.OutputPrivatekey(user_key,true,user_pass);
  out_f.open(user_key_file.c_str());
  out_f << user_key;
  out_f.close();

  // Load the request
  Arc::Credential user_eec;
  user_eec.InquireRequest(user_req_file.c_str(), true);
  user_eec.SetLifeTime(user_life);

  // Add subjectAltname extension to host cert
  std::string user_ext("EMAIL:user@localhost");
  user_eec.AddExtension("2.5.29.17", user_ext);

  // Sign request
  std::string user_dn("/O=Grid/OU=ARC/OU=localdomain/CN=User");
  ca.SignEECRequest(&user_eec, user_dn, user_cert_file.c_str());

  //Try to load user cert with wrong passphrase
  Arc::Credential user_cert_bad(user_cert_file,user_key_file,".",CAcert,"Bad password");

  //Load signed user cert
  Arc::Credential user_cert(user_cert_file,user_key_file,".",CAcert,user_pass);
  
  //Does the certificate chain verify?
  CPPUNIT_ASSERT(user_cert.GetVerification());
  // Did the signed cert get the right DN?
  CPPUNIT_ASSERT_EQUAL(user_dn,user_cert.GetDN());
  // Did the signed cert get the right identity - trivial for non-proxy?
  CPPUNIT_ASSERT_EQUAL(user_dn,user_cert.GetIdentityName());
  // Right type?
  CPPUNIT_ASSERT_EQUAL(user_cert.GetType(),ArcCredential::CERT_TYPE_EEC);

  // Get the lifetime
  CPPUNIT_ASSERT_EQUAL(Arc::Period(12*60*60),user_cert.GetLifeTime());

  // Proxy certificate
  BIO* req;
  req = BIO_new(BIO_s_mem());
  Arc::Credential proxy_req(t,0,keybits);
  proxy_req.GenerateRequest(req);

  //Signing side
  BIO* out; 
  out = BIO_new(BIO_s_mem());
  Arc::Credential proxy_cert;

  std::string proxy_cert_string;

  proxy_cert.InquireRequest(req);
  proxy_cert.SetProxyPolicy("rfc","independent","",-1);
  proxy_cert.SetLifeTime(Arc::Period(24*3600));
  user_cert.SignRequest(&proxy_cert, proxy_cert_string);

  BIO_free_all(req);
  BIO_free_all(out);

  std::string proxy_key_string;
  proxy_req.OutputPrivatekey(proxy_key_string);
  proxy_cert_string.append(proxy_key_string);

  std::string user_cert_string;
  user_cert.OutputCertificate(user_cert_string);
  proxy_cert_string.append(user_cert_string);

  //  std::string user_chain_string;
  //proxy_cert.OutputCertificateChain(user_chain_string);

  //  signer.OutputCertificateChain(signing_cert2_chain);

  std::string user_proxy_file("user_proxy.pem");
  out_f.open(user_proxy_file.c_str());
  out_f  <<  proxy_cert_string;
  out_f.close();

  //Load proxy
  Arc::Credential user_proxy(user_proxy_file,"",".","","");

  CPPUNIT_ASSERT_EQUAL(user_dn,user_proxy.GetIdentityName());
}

CPPUNIT_TEST_SUITE_REGISTRATION(CredentialTest);
