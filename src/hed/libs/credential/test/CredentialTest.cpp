#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <iostream>

#include <openssl/evp.h>
#include <arc/credential/CertUtil.h>
#include <arc/credential/Credential.h>
#include <arc/Utils.h>

class CredentialTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(CredentialTest);
  CPPUNIT_TEST(testCAcert);
  CPPUNIT_TEST(testhostcert);
  CPPUNIT_TEST(testusercert);
  CPPUNIT_TEST(testproxy);
  CPPUNIT_TEST(testproxy2proxy);
  CPPUNIT_TEST(testproxycertinfo);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown() {}
  void testCAcert();
  void testhostcert();
  void testusercert();
  void testproxy();
  void testproxy2proxy();
  void testproxycertinfo(){};
  
private:
  std::string srcdir;

  std::string CAcert;
  std::string CAkey;
  std::string CAserial;
  std::string CApassphrase;
  std::string CAdn;
  std::string CAconf; 
  std::string ca_ext_sect;
  std::string host_cert_ext_sect;
  std::string user_cert_ext_sect;

  std::string host_key_file;
  std::string host_cert_file;
  std::string host_dn;
 
  std::string user_key_file;
  std::string user_cert_file; 
  std::string user_passphrase;
  std::string user_dn;

  std::string user_proxy_file;
};

void CredentialTest::setUp() {
  srcdir = Arc::GetEnv("srcdir");
  if (srcdir.length() == 0)
    srcdir = ".";
  CAcert = "ca_cert.pem";
  CAkey = "ca_key.pem";
  CAserial = "ca_serial";
  CApassphrase = "capassword";
  CAdn = "/O=Grid/OU=ARC/CN=ARC CA";
  CAconf = srcdir + "/ca.cnf";
  ca_ext_sect = "v3_ca";
  host_cert_ext_sect = "host_cert";
  user_cert_ext_sect = "usr_cert"; 

  host_key_file = "host_key.pem";
  host_cert_file = "host_cert.pem";
  host_dn = "/O=Grid/OU=ARC/CN=localhost";
  
  user_key_file = "user_key.pem";
  user_cert_file = "user_cert.pem";
  user_passphrase = "userpassword";
  user_dn = "/O=Grid/OU=ARC/OU=localdomain/CN=User";
 
  user_proxy_file = "user_proxy.pem";
}

void CredentialTest::testCAcert() {

  // Create serial file
  std::ofstream out_f;
  out_f.open(CAserial.c_str());
  out_f << "00";
  out_f.close();

  // Create a CA certificate and its related key
  int ca_keybits = 2048;
  Arc::Time ca_t;
  Arc::Credential ca(ca_t, Arc::Period(365*24*3600), ca_keybits, "EEC");

  BIO* ca_req_bio = NULL;
  ca_req_bio = BIO_new(BIO_s_mem());
  ca.GenerateRequest(ca_req_bio);

  std::string subkeyid("hash");
  //ca.AddExtension("subjectKeyIdentifier", subkeyid.c_str());
  //ca.AddExtension("authorityKeyIdentifier", "keyid:always,issuer");
  //ca.AddExtension("basicConstraints", (char **)("CA:TRUE"));
  ca.SelfSignEECRequest(CAdn, CAconf.c_str(), ca_ext_sect, CAcert.c_str());

  std::ofstream out_key(CAkey.c_str(), std::ofstream::out);
  std::string ca_private_key;
  ca.OutputPrivatekey(ca_private_key);
  out_key.write(ca_private_key.c_str(), ca_private_key.size());
  out_key.close();

  // Load CA credential
  Arc::Credential ca2(CAcert, CAkey, CAserial, CAconf, "", CApassphrase);

  // Did we load a CA cert?
  CPPUNIT_ASSERT_EQUAL(ArcCredential::CERT_TYPE_CA, ca2.GetType());

  // Test if the DN is read properly
  CPPUNIT_ASSERT_EQUAL(CAdn, ca2.GetDN());

  if(ca_req_bio)BIO_free_all(ca_req_bio);
}

void CredentialTest::testhostcert() {

  // Default EEC values
  int keybits = 2048;
  Arc::Time t;

  // host cert signing
  std::string host_req_file("host_req.pem");

  Arc::Credential host_req(keybits);
  host_req.GenerateRequest(host_req_file.c_str());

  // Write private key to file for loading later - no passphrase for hosts
  std::string host_key;
  host_req.OutputPrivatekey(host_key);
  std::ofstream out_f;
  out_f.open(host_key_file.c_str());
  out_f << host_key;;
  out_f.close();

  // Load the request
  Arc::Credential host_eec;
  host_eec.InquireRequest(host_req_file.c_str(), true);
  Arc::Period host_life(30*24*3600);
  host_eec.SetLifeTime(host_life);

  // Add subjectAltname extension to host cert
  std::string host_ext("DNS:localhost");
  host_eec.AddExtension("2.5.29.17", host_ext, false, GEN_DNS);

  // Load CA credential
  Arc::Credential ca(CAcert, CAkey, CAserial, CAconf, host_cert_ext_sect, CApassphrase);

  // Sign request
  ca.SignEECRequest(&host_eec, host_dn, host_cert_file.c_str());

  //Load signed host cert
  Arc::Credential host_cert(host_cert_file, host_key_file, "",  CAcert);
  
  //Does the certificate chain verify?
  CPPUNIT_ASSERT(host_cert.GetVerification());
  // Did the signed cert get the right DN?
  CPPUNIT_ASSERT_EQUAL(host_dn,host_cert.GetDN());
  // Right type?
  CPPUNIT_ASSERT_EQUAL(ArcCredential::CERT_TYPE_EEC,host_cert.GetType());

}


void CredentialTest::testusercert() {

  // Default EEC values
  int keybits = 2048;
  Arc::Time t;

  // User cert signing
  std::string user_req_file("user_req.pem");
  Arc::Period user_life(30*24*3600);
  Arc::Credential user_req(t, user_life, keybits, "EEC");
  user_req.GenerateRequest(user_req_file.c_str());

  // Write private key to file for loading later
  std::string user_key;
  user_req.OutputPrivatekey(user_key,true,user_passphrase);
  std::ofstream out_f;
  out_f.open(user_key_file.c_str());
  out_f << user_key;
  out_f.close();

  // Here the original Credential object is used for signing;
  // We don't need to load the request, since we don't need to
  // inquire the X509_REQ 
  //Arc::Credential user_eec;
  //user_eec.InquireRequest(user_req_file.c_str(), true);
  //user_eec.SetLifeTime(user_life);

  // Add subjectAltname extension to host cert
  std::string user_ext("EMAIL:user@localhost");
  user_req.AddExtension("2.5.29.17", user_ext, false, GEN_EMAIL);

  // Load CA credential
  Arc::Credential ca(CAcert, CAkey, CAserial, CAconf, user_cert_ext_sect, CApassphrase);

  // Sign request
  ca.SignEECRequest(&user_req, user_dn, user_cert_file.c_str());

  //Try to load user cert with wrong passphrase
  Arc::Credential user_cert_bad(user_cert_file,user_key_file,".",CAcert,"Bad password");

  //Load signed user cert
  Arc::Credential user_cert(user_cert_file, user_key_file, ".", CAcert, user_passphrase);
  
  //Does the certificate chain verify?
  CPPUNIT_ASSERT(user_cert.GetVerification());

  // Did the signed cert get the right DN?
  CPPUNIT_ASSERT_EQUAL(user_dn,user_cert.GetDN());

  // Did the signed cert get the right identity - trivial for non-proxy?
  CPPUNIT_ASSERT_EQUAL(user_dn,user_cert.GetIdentityName());

  // Right type?
  CPPUNIT_ASSERT_EQUAL(ArcCredential::CERT_TYPE_EEC,user_cert.GetType());

  // Get the lifetime
  CPPUNIT_ASSERT_EQUAL(user_life, user_cert.GetLifeTime());

}

void CredentialTest::testproxy() {

  int keybits = 2048;
  Arc::Time t;
  
  // Generate certificate request
  BIO* req; 
  req = BIO_new(BIO_s_mem());
  Arc::Credential proxy_req(t,0,keybits);
  proxy_req.GenerateRequest(req);

  // Load EEC credential
  Arc::Credential user_cert(user_cert_file, user_key_file, ".", CAcert, user_passphrase);

  // Load the request 
  Arc::Credential proxy_cert;
  proxy_cert.InquireRequest(req);
  proxy_cert.SetProxyPolicy("rfc","independent","",-1);
  Arc::Period proxy_life(7*24*3600);
  proxy_cert.SetLifeTime(proxy_life);

  // Sign the request
  std::string proxy_cert_string;
  user_cert.SignRequest(&proxy_cert, proxy_cert_string);

  BIO_free_all(req);

  // Output proxy
  std::string proxy_key_string;
  proxy_req.OutputPrivatekey(proxy_key_string);
  proxy_cert_string.append(proxy_key_string);

  std::string user_cert_string;
  user_cert.OutputCertificate(user_cert_string);
  proxy_cert_string.append(user_cert_string);

  std::ofstream out_f;
  out_f.open(user_proxy_file.c_str());
  out_f<<proxy_cert_string;
  out_f.close();

  //Load proxy
  Arc::Credential user_proxy(user_proxy_file,"",".",CAcert);

  //Does the certificate chain verify?
  CPPUNIT_ASSERT(user_proxy.GetVerification());

  // Did we load a proxy with right type?
  CPPUNIT_ASSERT_EQUAL(ArcCredential::CERT_TYPE_RFC_INDEPENDENT_PROXY, user_proxy.GetType());

  // Did the proxy get the right identity (identified by the DN of the original user cert) 
  CPPUNIT_ASSERT_EQUAL(user_dn,user_proxy.GetIdentityName());

  // Get the lifetime
  CPPUNIT_ASSERT_EQUAL(proxy_life, user_proxy.GetLifeTime());

}

void CredentialTest::testproxy2proxy() {

  int keybits = 2048;
  int proxydepth = 10;

  Arc::Time t;

  //Generate a proxy certificate based on existing proxy certificate
  
  // Generate certificate request
  std::string user_req_file1("user_req1.pem");
  std::string user_proxy_file1("user_proxy1.pem");

  Arc::Credential request1(keybits);
  request1.GenerateRequest(user_req_file1.c_str());

  // Load the request
  Arc::Credential proxy_cert1;
  std::string signer_cert1 = user_proxy_file;
  Arc::Credential signer1(signer_cert1, "", ".", CAcert);
  proxy_cert1.InquireRequest(user_req_file1.c_str());
  proxy_cert1.SetProxyPolicy("rfc","independent","",proxydepth);
  Arc::Period proxy1_life(24*3600);
  proxy_cert1.SetLifeTime(proxy1_life);
  
  //Sign the request
  signer1.SignRequest(&proxy_cert1, user_proxy_file1.c_str());

  // Output a proxy
  std::string private_key1, signing_cert1, signing_cert1_chain;
  request1.OutputPrivatekey(private_key1);
  signer1.OutputCertificate(signing_cert1);
  signer1.OutputCertificateChain(signing_cert1_chain);
  std::ofstream out_f1(user_proxy_file1.c_str(), std::ofstream::app);
  out_f1.write(private_key1.c_str(), private_key1.size());
  out_f1.write(signing_cert1.c_str(), signing_cert1.size());
  out_f1.write(signing_cert1_chain.c_str(), signing_cert1_chain.size());
  out_f1.close();

  //Load the proxy
  Arc::Credential user_proxy1(user_proxy_file1,"",".",CAcert);

  //Does the certificate chain verify?
  CPPUNIT_ASSERT(user_proxy1.GetVerification());

  // Did we load a proxy with right type?
  CPPUNIT_ASSERT_EQUAL(ArcCredential::CERT_TYPE_RFC_INDEPENDENT_PROXY, user_proxy1.GetType());

  // Did the proxy get the right identity (identified by the DN of the original user cert)
  CPPUNIT_ASSERT_EQUAL(user_dn,user_proxy1.GetIdentityName());

  // Get the lifetime
  CPPUNIT_ASSERT_EQUAL(proxy1_life, user_proxy1.GetLifeTime());



  //Generate one more proxy based on existing proxy just generated
 
  // Generate certificate request
  std::string user_req_file2("user_req2.pem");
  std::string user_proxy_file2("user_proxy2.pem");
  Arc::Credential request2(t, Arc::Period(12*3600), keybits);
  request2.GenerateRequest(user_req_file2.c_str());

  // Load the request
  Arc::Credential proxy_cert2;
  std::string signer_cert2 = user_proxy_file1;
  Arc::Credential signer2(signer_cert2, "", ".", CAcert);
  proxy_cert2.InquireRequest(user_req_file2.c_str());
  proxy_cert2.SetProxyPolicy("rfc","independent","",proxydepth-3);
  Arc::Period proxy2_life(8*3600);
  proxy_cert2.SetLifeTime(proxy2_life);
 
  //Sign the request
  signer2.SignRequest(&proxy_cert2, user_proxy_file2.c_str());

  // Output a proxy
  std::string private_key2, signing_cert2, signing_cert2_chain;
  request2.OutputPrivatekey(private_key2);
  signer2.OutputCertificate(signing_cert2);
  signer2.OutputCertificateChain(signing_cert2_chain);
  std::ofstream out_f2(user_proxy_file2.c_str(), std::ofstream::app);
  out_f2.write(private_key2.c_str(), private_key2.size());
  out_f2.write(signing_cert2.c_str(), signing_cert2.size());
  out_f2.write(signing_cert2_chain.c_str(), signing_cert2_chain.size());
  out_f2.close();

  //Load the proxy
  Arc::Credential user_proxy2(user_proxy_file2,"","",CAcert);


  //Does the certificate chain verify?
  CPPUNIT_ASSERT(user_proxy2.GetVerification());

  // Did we load a proxy with right type?
  CPPUNIT_ASSERT_EQUAL(ArcCredential::CERT_TYPE_RFC_INDEPENDENT_PROXY, user_proxy2.GetType());

  // Did the proxy get the right identity (identified by the DN of the original user cert)
  CPPUNIT_ASSERT_EQUAL(user_dn,user_proxy2.GetIdentityName());

  // Get the lifetime
  CPPUNIT_ASSERT_EQUAL(proxy2_life, user_proxy2.GetLifeTime());

}

CPPUNIT_TEST_SUITE_REGISTRATION(CredentialTest);
