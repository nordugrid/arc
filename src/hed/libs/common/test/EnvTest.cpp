#include <cppunit/extensions/HelperMacros.h>

#include <arc/Utils.h>

class EnvTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(EnvTest);
  CPPUNIT_TEST(TestEnv);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestEnv();

};

void EnvTest::setUp() {
}

void EnvTest::tearDown() {
}

void EnvTest::TestEnv() {
  std::string val;
  bool found = false;

  Arc::SetEnv("TEST_ENV_VAR","TEST_ENV_VALUE");
  CPPUNIT_ASSERT_EQUAL(std::string("TEST_ENV_VALUE"), Arc::GetEnv("TEST_ENV_VAR",found));
  CPPUNIT_ASSERT_EQUAL(true, found);
  Arc::UnsetEnv("TEST_ENV_VAR");
  CPPUNIT_ASSERT_EQUAL(std::string(""), Arc::GetEnv("TEST_ENV_VAR",found));
  CPPUNIT_ASSERT_EQUAL(false, found);

  Arc::SetEnv("TEST_ENV_VAR","TEST_ENV_VALUE2");
  Arc::SetEnv("TEST_ENV_VAR","TEST_ENV_VALUE3");
  Arc::SetEnv("TEST_ENV_VAR","TEST_ENV_VALUE4", false);
  CPPUNIT_ASSERT_EQUAL(std::string("TEST_ENV_VALUE3"), Arc::GetEnv("TEST_ENV_VAR",found));

}

CPPUNIT_TEST_SUITE_REGISTRATION(EnvTest);
