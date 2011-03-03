#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <fstream>

#include <arc/UserConfig.h>

class UserConfigTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UserConfigTest);
  CPPUNIT_TEST(AliasTest);
  CPPUNIT_TEST_SUITE_END();

public:
  UserConfigTest() : uc(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)), conffile("test-client.conf") {}

  void AliasTest();

  void setUp() {}
  void tearDown() {}

private:
  Arc::UserConfig uc;
  const std::string conffile;
};

void UserConfigTest::AliasTest()
{
  std::ofstream f(conffile.c_str(), std::ifstream::trunc);
  f << "[ alias ]" << std::endl;
  f << "a = computing:TEST:http://a.org" << std::endl;
  f << "b = computing:TEST:http://b.org" << std::endl;
  f << "c = a b computing:TEST:http://c.org" << std::endl;
  f << "invalid = compute:TEST:http://invalid.org" << std::endl;
  f << "i = index:TEST:http://i.org" << std::endl;
  f << "j = index:TEST:http://j.org" << std::endl;
  f << "k = i j index:TEST:http://k.org" << std::endl;
  f << "mixed = a b i j" << std::endl;
  f << "loop = a b link" << std::endl;
  f << "link = loop" << std::endl;
  f.close();

  uc.LoadConfigurationFile(conffile);

  std::list<std::string> s;

  // Checking normal computing alias.
  s.push_back("a");
  s.push_back("b");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://b.org", s.back());
  s.clear();

  // Checking computing alias referring to computing alias.
  s.push_back("c");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(3, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  s.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://b.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://c.org", s.back());
  s.clear();

  // Checking normal index alias.
  s.push_back("i");
  s.push_back("j");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://i.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://j.org", s.back());
  s.clear();

  // Checking index alias referring to computing alias.
  s.push_back("k");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(3, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://i.org", s.front());
  s.pop_front();
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://j.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://k.org", s.back());
  s.clear();

  // Checking alias referring to an alias.
  s.push_back("a");
  s.push_back("invalid");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(1, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  s.clear();

  // Checking mixed alias.
  s.push_back("mixed");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://a.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://b.org", s.back());
  s.clear();
  s.push_back("mixed");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(2, (int)s.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://i.org", s.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"TEST:http://j.org", s.back());
  s.clear();
  s.push_back("a");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::INDEX));
  CPPUNIT_ASSERT_EQUAL(0, (int)s.size());
  s.clear();
  s.push_back("i");
  CPPUNIT_ASSERT(uc.ResolveAliases(s, Arc::COMPUTING));
  CPPUNIT_ASSERT_EQUAL(0, (int)s.size());
  s.clear();

  // Check if loops are detected.
  s.push_back("loop");
  CPPUNIT_ASSERT(!uc.ResolveAliases(s, Arc::COMPUTING));
  s.clear();

  // Specifying an undefined alias should return false.
  s.push_back("undefined");
  CPPUNIT_ASSERT(!uc.ResolveAliases(s, Arc::COMPUTING));
  s.clear();

  remove(conffile.c_str());
}

CPPUNIT_TEST_SUITE_REGISTRATION(UserConfigTest);
