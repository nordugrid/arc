#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Job.h>
#include <arc/client/Submitter.h>

class SubmitterTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SubmitterTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  SubmitterTest();
  ~SubmitterTest() { delete s; }

  void setUp() {}
  void tearDown() {}

  void LoadTest();

private:
  Arc::Submitter *s;
  Arc::SubmitterLoader *sl;
  Arc::UserConfig usercfg;
};

SubmitterTest::SubmitterTest() : s(NULL), usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {
  Arc::SetEnv("ARC_PLUGIN_PATH", ".libs");

  sl = new Arc::SubmitterLoader();
}

void SubmitterTest::LoadTest()
{
  s = sl->load("", usercfg);
  CPPUNIT_ASSERT(s == NULL);

  s = sl->load("NON-EXISTENT", usercfg);
  CPPUNIT_ASSERT(s == NULL);

  s = sl->load("TEST", usercfg);
  CPPUNIT_ASSERT(s != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(SubmitterTest);
