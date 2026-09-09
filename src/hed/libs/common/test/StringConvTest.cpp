#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <limits>

#include <arc/StringConv.h>

class StringConvTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(StringConvTest);
  CPPUNIT_TEST(TestStringConv);
  CPPUNIT_TEST(TestURIEncode);
  CPPUNIT_TEST(TestIntegers);
  CPPUNIT_TEST(TestJoin);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestStringConv();
  void TestURIEncode();
  void TestIntegers();
  void TestJoin();
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
  CPPUNIT_ASSERT_EQUAL(std::string("http%3A//localhost/data/file1"), out);
  CPPUNIT_ASSERT_EQUAL(in, Arc::uri_unencode(out));

  // complex case
  in = "http://localhost:80/data/file with spaces&name=value&symbols=()!%*$";
  out = Arc::uri_encode(in, false);
  CPPUNIT_ASSERT_EQUAL(std::string("http%3A//localhost%3A80/data/file%20with%20spaces%26name%3Dvalue%26symbols%3D%28%29%21%25%2A%24"), out);
  CPPUNIT_ASSERT_EQUAL(in, Arc::uri_unencode(out));

  out = Arc::uri_encode(in, true);
  CPPUNIT_ASSERT_EQUAL(std::string("http%3A%2F%2Flocalhost%3A80%2Fdata%2Ffile%20with%20spaces%26name%3Dvalue%26symbols%3D%28%29%21%25%2A%24"), out);
  CPPUNIT_ASSERT_EQUAL(in, Arc::uri_unencode(out));

}

void StringConvTest::TestIntegers() {
  int n = 12345;

  CPPUNIT_ASSERT_EQUAL(std::string("12345"),Arc::inttostr(n));
  CPPUNIT_ASSERT_EQUAL(std::string("-12345"),Arc::inttostr(-n));

  CPPUNIT_ASSERT_EQUAL(std::string("0000012345"),Arc::inttostr(n,10,10));

  CPPUNIT_ASSERT_EQUAL(std::string("343340"),Arc::inttostr(n,5,2));
  CPPUNIT_ASSERT_EQUAL(std::string("1ah5"),Arc::inttostr(n,20));

  CPPUNIT_ASSERT(Arc::strtoint("12345",n));
  CPPUNIT_ASSERT_EQUAL(12345,n);
  CPPUNIT_ASSERT(Arc::strtoint("+12345",n));
  CPPUNIT_ASSERT_EQUAL(12345,n);
  CPPUNIT_ASSERT(Arc::strtoint("-12345",n));
  CPPUNIT_ASSERT_EQUAL(-12345,n);
  CPPUNIT_ASSERT(Arc::strtoint("343340",n,5));
  CPPUNIT_ASSERT_EQUAL(12345,n);
  CPPUNIT_ASSERT(Arc::strtoint("1ah5",n,20));
  CPPUNIT_ASSERT_EQUAL(12345,n);
  CPPUNIT_ASSERT(Arc::strtoint("1AH5",n,20));
  CPPUNIT_ASSERT_EQUAL(12345,n);

  n = 7;
  CPPUNIT_ASSERT(!Arc::strtoint("+",n));
  CPPUNIT_ASSERT_EQUAL(7,n);
  CPPUNIT_ASSERT(!Arc::strtoint("12x",n));
  CPPUNIT_ASSERT(!Arc::strtoint("2",n,2));

  const int int_min = std::numeric_limits<int>::min();
  const int int_max = std::numeric_limits<int>::max();
  CPPUNIT_ASSERT(Arc::strtoint(Arc::tostring(int_min),n));
  CPPUNIT_ASSERT_EQUAL(int_min,n);
  CPPUNIT_ASSERT(Arc::strtoint(Arc::tostring(int_max),n));
  CPPUNIT_ASSERT_EQUAL(int_max,n);
  CPPUNIT_ASSERT(!Arc::strtoint(Arc::tostring(static_cast<long long>(int_max) + 1),n));
  CPPUNIT_ASSERT(!Arc::strtoint(Arc::tostring(static_cast<long long>(int_min) - 1),n));

  unsigned int un = 7;
  CPPUNIT_ASSERT(!Arc::strtoint("-1",un));
  CPPUNIT_ASSERT_EQUAL(7U,un);

  unsigned long long ull = 0;
  const std::string ull_max = Arc::tostring(std::numeric_limits<unsigned long long>::max());
  CPPUNIT_ASSERT(Arc::strtoint(ull_max,ull));
  CPPUNIT_ASSERT_EQUAL(std::numeric_limits<unsigned long long>::max(),ull);
  CPPUNIT_ASSERT(!Arc::strtoint(ull_max + "0",ull));

  const long long ll_min = std::numeric_limits<long long>::min();
  long long ll = 0;
  CPPUNIT_ASSERT(Arc::strtoint(Arc::tostring(ll_min),ll));
  CPPUNIT_ASSERT_EQUAL(ll_min,ll);
  CPPUNIT_ASSERT_EQUAL(Arc::tostring(ll_min),Arc::inttostr(ll_min));
}

void StringConvTest::TestJoin() {

  std::list<std::string> strlist;
  CPPUNIT_ASSERT(Arc::join(strlist, " ").empty());

  strlist.push_back("test");
  CPPUNIT_ASSERT_EQUAL(std::string("test"), Arc::join(strlist, " "));

  strlist.push_back("again");
  CPPUNIT_ASSERT_EQUAL(std::string("test again"), Arc::join(strlist, " "));

  strlist.push_back("twice");
  CPPUNIT_ASSERT_EQUAL(std::string("test,again,twice"), Arc::join(strlist, ","));
}

CPPUNIT_TEST_SUITE_REGISTRATION(StringConvTest);
