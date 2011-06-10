#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>

class StringConvTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(StringConvTest);
  CPPUNIT_TEST(TestStringConv);
  CPPUNIT_TEST(TestURIEncode);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestStringConv();
  void TestURIEncode();
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
  out = Arc::escape_chars(in,"13579",'#',false);
  CPPUNIT_ASSERT_EQUAL(std::string("#12#34#56#78#90"), out);
  out += "###";
  out = Arc::unescape_chars(out,'#');
  CPPUNIT_ASSERT_EQUAL(in + "##", out);
  out = Arc::escape_chars(in,"13579",'#',false,Arc::escape_hex);
  CPPUNIT_ASSERT_EQUAL(std::string("#312#334#356#378#390"), out);
  out = Arc::unescape_chars(out,'#',Arc::escape_hex);
  CPPUNIT_ASSERT_EQUAL(in, out);
  out = Arc::escape_chars(in,"13579",'#',false,Arc::escape_octal);
  CPPUNIT_ASSERT_EQUAL(std::string("#0612#0634#0656#0678#0710"), out);
  out = Arc::unescape_chars(out,'#',Arc::escape_octal);
  CPPUNIT_ASSERT_EQUAL(in, out);
  out = Arc::escape_chars(in,"13579",'#',true,Arc::escape_hex);
  CPPUNIT_ASSERT_EQUAL(std::string("1#323#345#367#389#30"), out);
  out = Arc::unescape_chars(out,'#',Arc::escape_hex);
  CPPUNIT_ASSERT_EQUAL(in, out);
}

void StringConvTest::TestURIEncode() {
  std::string in;
  std::string out;

  // simple URL
  in = "http://localhost/data/file1";
  out = Arc::uri_encode(in, false);
  CPPUNIT_ASSERT_EQUAL(std::string("http%3a//localhost/data/file1"), out);
  CPPUNIT_ASSERT_EQUAL(in, Arc::uri_unencode(out));

  // complex case
  in = "http://localhost:80/data/file with spaces&name=value&symbols=()!%*$";
  out = Arc::uri_encode(in, false);
  CPPUNIT_ASSERT_EQUAL(std::string("http%3a//localhost%3a80/data/file%20with%20spaces%26name%3dvalue%26symbols%3d%28%29%21%25%2a%24"), out);
  CPPUNIT_ASSERT_EQUAL(in, Arc::uri_unencode(out));

  out = Arc::uri_encode(in, true);
  CPPUNIT_ASSERT_EQUAL(std::string("http%3a%2f%2flocalhost%3a80%2fdata%2ffile%20with%20spaces%26name%3dvalue%26symbols%3d%28%29%21%25%2a%24"), out);
  CPPUNIT_ASSERT_EQUAL(in, Arc::uri_unencode(out));

}

CPPUNIT_TEST_SUITE_REGISTRATION(StringConvTest);
