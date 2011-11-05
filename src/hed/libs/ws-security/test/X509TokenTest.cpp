// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/ws-security/X509Token.h>
#include <arc/xmlsec/XmlSecUtils.h>

class X509TokenTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(X509TokenTest);
  CPPUNIT_TEST(TestSignatureToken);
  CPPUNIT_TEST(TestEncryptionToken);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestSignatureToken();
  void TestEncryptionToken();

private:
  std::string xml;
  std::string certfile;
  std::string keyfile;
};


void X509TokenTest::setUp() {
  xml = std::string("\
<S:Envelope xmlns:S=\"http://www.w3.org/2003/05/soap-envelope\"\
  xmlns:wsa=\"http://www.w3.org/2005/08/addressing\"\
  xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">\
  <S:Header>\
  </S:Header>\
  <S:Body>\
    <f:Delete xmlns:f=\"http://example.com/fabrikam\">\
       <maxCount>42</maxCount>\
    </f:Delete>\
  </S:Body>\
</S:Envelope>\
");

  Arc::init_xmlsec();

  certfile = "../../credential/test/host_cert.pem";
  keyfile = "../../credential/test/host_key.pem";

}

void X509TokenTest::tearDown() {
  Arc::final_xmlsec();
}

void X509TokenTest::TestSignatureToken() {
  Arc::SOAPEnvelope soap1(xml);
  CPPUNIT_ASSERT((bool)soap1);
  Arc::X509Token xt1(soap1, certfile, keyfile);
  CPPUNIT_ASSERT((bool)xt1);
  std::string str;
  xt1.GetXML(str);

  Arc::SOAPEnvelope soap2(str);
  CPPUNIT_ASSERT((bool)soap2);
  Arc::X509Token xt2(soap2);
  CPPUNIT_ASSERT((bool)xt2);
  CPPUNIT_ASSERT(xt2.Authenticate());
  CPPUNIT_ASSERT(xt2.Authenticate("../../credential/test/ca_cert.pem", ""));
}

void X509TokenTest::TestEncryptionToken() {
  Arc::SOAPEnvelope soap1(xml);
  CPPUNIT_ASSERT((bool)soap1);
  Arc::X509Token xt1(soap1, certfile, "", Arc::X509Token::Encryption);
  CPPUNIT_ASSERT((bool)xt1);
  std::string str;
  xt1.GetXML(str);

  Arc::SOAPEnvelope soap2(str);
  CPPUNIT_ASSERT((bool)soap2);
  Arc::X509Token xt2(soap2, keyfile);
  CPPUNIT_ASSERT((bool)xt2);
  
  Arc::XMLNode node1(xml);
  CPPUNIT_ASSERT((bool)node1);
  std::string str1; node1["Body"].Child().GetXML(str1);
  CPPUNIT_ASSERT(!str1.empty());

  std::string str2;
  xt2.Child().GetXML(str2); 

  CPPUNIT_ASSERT_EQUAL(str1, str2);
}

CPPUNIT_TEST_SUITE_REGISTRATION(X509TokenTest);
