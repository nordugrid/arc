#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Base64.h>

class Base64Test
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(Base64Test);
  CPPUNIT_TEST(TestBase64);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestBase64();

};

void Base64Test::setUp() {
}

void Base64Test::tearDown() {
}

void Base64Test::TestBase64() {
  std::string val;

  std::string plain_str("base64 to test 1234567890 abcdefghijklmnopqrstuvwxyz");
  std::string encoded_str("YmFzZTY0IHRvIHRlc3QgMTIzNDU2Nzg5MCBhYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5eg==");
  std::string str;

  // encode
  str = Arc::Base64::encode(plain_str);

  CPPUNIT_ASSERT_EQUAL(encoded_str, str);

  str.insert(16,"\r\n");
  str.insert(34,"\r\n");

  // decode
  str = Arc::Base64::decode(str);

  CPPUNIT_ASSERT_EQUAL(plain_str, str);
  
}

CPPUNIT_TEST_SUITE_REGISTRATION(Base64Test);
