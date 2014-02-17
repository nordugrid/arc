#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <arc/ArcRegex.h>

class ArcRegexTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ArcRegexTest);
  CPPUNIT_TEST(TestRegex);
  CPPUNIT_TEST(TestCaseInsensitive);
  CPPUNIT_TEST(TestSubExpression);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestRegex();
  void TestCaseInsensitive();
  void TestSubExpression();
};

void ArcRegexTest::TestRegex() {

  std::list<std::string> match, unmatch;

  std::string s = "the cat sat on the mat";
  std::string r = "cat";
  Arc::RegularExpression simplerx(r);
  CPPUNIT_ASSERT(simplerx.isOk());

  CPPUNIT_ASSERT(!simplerx.match(s)); // must match whole string for success
  CPPUNIT_ASSERT(simplerx.match(s, unmatch, match));
  CPPUNIT_ASSERT_EQUAL(1, (int)match.size());
  std::list<std::string>::iterator i = match.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("cat"), *i);

  CPPUNIT_ASSERT_EQUAL(2, (int)unmatch.size());
  i = unmatch.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("the "), *i); i++;
  CPPUNIT_ASSERT_EQUAL(std::string(" sat on the mat"), *i);

  r = "([a-zA-Z0-9_\\\\-]*)=\"([a-zA-Z0-9_\\\\-]*)\"";
  Arc::RegularExpression rx1(r);
  CPPUNIT_ASSERT(rx1.isOk());
  Arc::RegularExpression rx2 = rx1;
  CPPUNIT_ASSERT(rx2.isOk());

  CPPUNIT_ASSERT(rx1.hasPattern("([a-zA-Z0-9_\\\\-]*)=\"([a-zA-Z0-9_\\\\-]*)\""));
  CPPUNIT_ASSERT(rx1.hasPattern(r));
  CPPUNIT_ASSERT(!rx1.hasPattern("abcd"));
  CPPUNIT_ASSERT(!rx1.match("keyvalue"));

  CPPUNIT_ASSERT(rx1.match("key=\"value\"", unmatch, match));
  CPPUNIT_ASSERT_EQUAL(3, (int)match.size());
  i = match.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("key=\"value\""), *i); i++;
  CPPUNIT_ASSERT_EQUAL(std::string("key"), *i); i++;
  CPPUNIT_ASSERT_EQUAL(std::string("value"), *i);

  CPPUNIT_ASSERT_EQUAL(2, (int)unmatch.size());
  i = unmatch.begin();
  CPPUNIT_ASSERT_EQUAL(std::string("=\""), *i); i++;
  CPPUNIT_ASSERT_EQUAL(std::string("\""), *i);

  Arc::RegularExpression empty_regexp;
  CPPUNIT_ASSERT(!empty_regexp.isOk());

}

void ArcRegexTest::TestCaseInsensitive() {
  Arc::RegularExpression cis("foo", true);
  CPPUNIT_ASSERT(cis.isOk());
  CPPUNIT_ASSERT(cis.match("FOO"));
  CPPUNIT_ASSERT(cis.match("foo"));
  CPPUNIT_ASSERT(cis.match("fOo"));
  
  Arc::RegularExpression cs("foo");
  CPPUNIT_ASSERT(cs.isOk());
  CPPUNIT_ASSERT(!cs.match("FOO"));
  CPPUNIT_ASSERT(cs.match("foo"));
  CPPUNIT_ASSERT(!cs.match("fOo"));
}

void ArcRegexTest::TestSubExpression() {
  Arc::RegularExpression r("^(abc)?def(ghi)jkl(mno)?$");
  CPPUNIT_ASSERT(r.isOk());
  std::vector<std::string> matches;
  
  CPPUNIT_ASSERT(r.match("defghijkl", matches));
  CPPUNIT_ASSERT_EQUAL(3, (int)matches.size());
  CPPUNIT_ASSERT(matches[0].empty());
  CPPUNIT_ASSERT_EQUAL((std::string)"ghi", matches[1]);
  CPPUNIT_ASSERT(matches[2].empty());
  matches.clear();
  
  CPPUNIT_ASSERT(r.match("abcdefghijkl", matches));
  CPPUNIT_ASSERT_EQUAL(3, (int)matches.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"abc", matches[0]);
  CPPUNIT_ASSERT_EQUAL((std::string)"ghi", matches[1]);
  CPPUNIT_ASSERT(matches[2].empty());
  matches.clear();
  
  CPPUNIT_ASSERT(r.match("defghijklmno", matches));
  CPPUNIT_ASSERT_EQUAL(3, (int)matches.size());
  CPPUNIT_ASSERT(matches[0].empty());
  CPPUNIT_ASSERT_EQUAL((std::string)"ghi", matches[1]);
  CPPUNIT_ASSERT_EQUAL((std::string)"mno", matches[2]);
  matches.clear();
  
  CPPUNIT_ASSERT(r.match("abcdefghijklmno", matches));
  CPPUNIT_ASSERT_EQUAL(3, (int)matches.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"abc", matches[0]);
  CPPUNIT_ASSERT_EQUAL((std::string)"ghi", matches[1]);
  CPPUNIT_ASSERT_EQUAL((std::string)"mno", matches[2]);
  matches.clear();

  CPPUNIT_ASSERT(!r.match("defjkl", matches));
  matches.clear();
}

CPPUNIT_TEST_SUITE_REGISTRATION(ArcRegexTest);
