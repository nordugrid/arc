#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>

class StringConvTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(StringConvTest);
  CPPUNIT_TEST(TestStringConv);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestStringConv();
};

void StringConvTest::TestStringConv() {

  std::string in;
  std::string out;

  in = "aBcDeFgHiJkLmN";
  out = Arc::lower(in);
  CPPUNIT_ASSERT_EQUAL(std::string("abcdefghijklmn"), out);
  out = Arc::upper(in);
  CPPUNIT_ASSERT_EQUAL(std::string("ABCDEFGHIJKLMN"), out);

  in = "####0123456789++++";
  out = Arc::trim(in,"#+");
  CPPUNIT_ASSERT_EQUAL(std::string("0123456789"), out);

  in = "0123\n\t   \t\n\n\n\n456789\n\t\n";
  out = Arc::strip(in);
  CPPUNIT_ASSERT_EQUAL(std::string("0123\n456789"), out);

  in = "1234567890";
  out = Arc::escape_chars(in,"13579",'#');
  CPPUNIT_ASSERT_EQUAL(std::string("#12#34#56#78#90"), out);
  out += "###";
  out = Arc::unescape_chars(out,'#');
  CPPUNIT_ASSERT_EQUAL(in + "##", out);
  out = Arc::escape_chars(in,"13579",'#',Arc::escape_hex);
  CPPUNIT_ASSERT_EQUAL(std::string("#312#334#356#378#390"), out);
  out = Arc::unescape_chars(out,'#',Arc::escape_hex);
  CPPUNIT_ASSERT_EQUAL(in, out);
  out = Arc::escape_chars(in,"13579",'#',Arc::escape_octal);
  CPPUNIT_ASSERT_EQUAL(std::string("#0612#0634#0656#0678#0710"), out);
  out = Arc::unescape_chars(out,'#',Arc::escape_octal);
  CPPUNIT_ASSERT_EQUAL(in, out);
}

CPPUNIT_TEST_SUITE_REGISTRATION(StringConvTest);
