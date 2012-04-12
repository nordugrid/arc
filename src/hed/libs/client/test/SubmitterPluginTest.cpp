#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Job.h>
#include <arc/client/SubmitterPlugin.h>
#include <arc/Thread.h>

class SubmitterPluginTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(SubmitterPluginTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  SubmitterPluginTest();
  ~SubmitterPluginTest() { delete sl; }

  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }

  void LoadTest();

private:
  Arc::SubmitterPlugin *s;
  Arc::SubmitterPluginLoader *sl;
  Arc::UserConfig usercfg;
};

SubmitterPluginTest::SubmitterPluginTest() : s(NULL), usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {
  sl = new Arc::SubmitterPluginLoader();
}

void SubmitterPluginTest::LoadTest()
{
  s = sl->load("", usercfg);
  CPPUNIT_ASSERT(s == NULL);

  s = sl->load("NON-EXISTENT", usercfg);
  CPPUNIT_ASSERT(s == NULL);

  s = sl->load("TEST", usercfg);
  CPPUNIT_ASSERT(s != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(SubmitterPluginTest);
