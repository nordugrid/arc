// -*- indent-tabs-mode: nil -*-

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/ws-security/UsernameToken.h>

class UsernameTokenTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UsernameTokenTest);
  CPPUNIT_TEST(TestTokenGenerationWithIteration);
  CPPUNIT_TEST(TestTokenGenerationWithPassword);
  CPPUNIT_TEST(TestTokenParsing);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestTokenGenerationWithIteration();
  void TestTokenGenerationWithPassword();
  void TestTokenParsing();

private:
  std::string xml;
  std::string username;
  std::string password;
  std::string uid;
};


void UsernameTokenTest::setUp() {
  xml = std::string("\
<S:Envelope xmlns:S=\"http://www.w3.org/2003/05/soap-envelope\"\
  xmlns:wsa=\"http://www.w3.org/2005/08/addressing\">\
  <S:Header>\
  </S:Header>\
  <S:Body>\
    <f:Delete xmlns:f=\"http://example.com/fabrikam\">\
       <maxCount>42</maxCount>\
    </f:Delete>\
  </S:Body>\
</S:Envelope>\
");

  username = "user";
  password = "pass";
  uid = "test-1";
}

void UsernameTokenTest::tearDown() {
}

void UsernameTokenTest::TestTokenGenerationWithPassword() {
  Arc::SOAPEnvelope soap(xml);
  Arc::UsernameToken ut(soap, username, password, uid, Arc::UsernameToken::PasswordDigest);
  CPPUNIT_ASSERT((bool)ut);

  std::string derived_key;
  CPPUNIT_ASSERT((bool)ut);
  CPPUNIT_ASSERT(ut.Authenticate(password,derived_key));
  CPPUNIT_ASSERT_EQUAL(ut.Username(), username);
  CPPUNIT_ASSERT(derived_key.empty());
}

void UsernameTokenTest::TestTokenGenerationWithIteration() {
  Arc::SOAPEnvelope soap(xml);
  Arc::UsernameToken ut(soap, username, uid, true, 150);

  std::string derived_key;
  CPPUNIT_ASSERT((bool)ut);
  CPPUNIT_ASSERT(ut.Authenticate(password,derived_key));
  CPPUNIT_ASSERT_EQUAL(ut.Username(), username);
  CPPUNIT_ASSERT(!derived_key.empty());
}

void UsernameTokenTest::TestTokenParsing() {
  Arc::SOAPEnvelope soap(xml);
  Arc::UsernameToken ut1(soap, username, uid, true, 150);
  std::string derived_key1;
  CPPUNIT_ASSERT((bool)ut1);
  CPPUNIT_ASSERT(ut1.Authenticate(password,derived_key1));
  CPPUNIT_ASSERT_EQUAL(ut1.Username(), username);
  CPPUNIT_ASSERT(!derived_key1.empty());

  Arc::UsernameToken ut2(soap);
  std::string derived_key2;
  CPPUNIT_ASSERT((bool)ut2);
  CPPUNIT_ASSERT(ut2.Authenticate(password,derived_key2));
  CPPUNIT_ASSERT_EQUAL(ut2.Username(), username);
  CPPUNIT_ASSERT(!derived_key2.empty());

  CPPUNIT_ASSERT_EQUAL(derived_key1, derived_key2);
}

CPPUNIT_TEST_SUITE_REGISTRATION(UsernameTokenTest);
