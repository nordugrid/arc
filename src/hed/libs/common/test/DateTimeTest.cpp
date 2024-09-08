#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Base64.h>
#include <arc/DateTime.h>

class DateTimeTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(DateTimeTest);
  CPPUNIT_TEST(TestDateTime);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestDateTime();

};

void DateTimeTest::setUp() {
}

void DateTimeTest::tearDown() {
}

void DateTimeTest::TestDateTime() {
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

CPPUNIT_TEST_SUITE_REGISTRATION(DateTimeTest);
