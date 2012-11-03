#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobControllerPlugin.h>
#include <arc/Thread.h>

class JobControllerPluginTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobControllerPluginTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobControllerPluginTest();
  ~JobControllerPluginTest() { delete jcl; }

  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }

  void LoadTest();

private:
  Arc::JobControllerPlugin *jc;
  Arc::JobControllerPluginLoader *jcl;
  Arc::UserConfig usercfg;
};

JobControllerPluginTest::JobControllerPluginTest() : jc(NULL), usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {
  jcl = new Arc::JobControllerPluginLoader();
}

void JobControllerPluginTest::LoadTest()
{
  jc = jcl->load("", usercfg);
  CPPUNIT_ASSERT(jc == NULL);

  jc = jcl->load("NON-EXISTENT", usercfg);
  CPPUNIT_ASSERT(jc == NULL);

  jc = jcl->load("TEST", usercfg);
  CPPUNIT_ASSERT(jc != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobControllerPluginTest);
