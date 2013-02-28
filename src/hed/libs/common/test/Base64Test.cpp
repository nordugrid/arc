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
  bool found = false;

  std::string plain_str("base64 to test 1234567890 abcdefghijklmnopqrstuvwxyz");
  std::string encoded_str;

  // encode
  encoded_str = Arc::Base64::encode(plain_str);

  // decode
  std::string decoded_str = Arc::Base64::decode(encoded_str);

  CPPUNIT_ASSERT_EQUAL(plain_str.length(), decoded_str.length());
  CPPUNIT_ASSERT(!plain_str.compare(decoded_str));
  
}

CPPUNIT_TEST_SUITE_REGISTRATION(Base64Test);
