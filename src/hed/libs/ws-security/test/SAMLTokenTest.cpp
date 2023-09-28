// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/ws-security/SAMLToken.h>
#include <arc/xmlsec/XmlSecUtils.h>

class SAMLTokenTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SAMLTokenTest);
  CPPUNIT_TEST(TestSAML2Token);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestSAML2Token();

private:
  std::string xml;
  std::string certfile;
  std::string keyfile;
};


void SAMLTokenTest::setUp() {
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

void SAMLTokenTest::tearDown() {
  Arc::final_xmlsec();
}

void SAMLTokenTest::TestSAML2Token() {
  Arc::SOAPEnvelope soap1(xml);
  CPPUNIT_ASSERT((bool)soap1);
  Arc::SAMLToken st1(soap1, certfile, keyfile, Arc::SAMLToken::SAML2);
  CPPUNIT_ASSERT((bool)st1);

  std::string str;
  st1.GetXML(str);

  Arc::SOAPEnvelope soap2(str);
  CPPUNIT_ASSERT((bool)soap2);
  Arc::SAMLToken st2(soap2);
  CPPUNIT_ASSERT((bool)st2);
  CPPUNIT_ASSERT(st2.Authenticate());
  CPPUNIT_ASSERT(st2.Authenticate("../../credential/test/ca_cert.pem", "", false));
}

CPPUNIT_TEST_SUITE_REGISTRATION(SAMLTokenTest);
