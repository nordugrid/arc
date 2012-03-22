#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Job.h>
#include <arc/client/JobController.h>
#include <arc/Thread.h>

class JobControllerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobControllerTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobControllerTest();
  ~JobControllerTest() { delete jcl; }

  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }

  void LoadTest();

private:
  Arc::JobController *jc;
  Arc::JobControllerLoader *jcl;
  Arc::UserConfig usercfg;
};

JobControllerTest::JobControllerTest() : jc(NULL), usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {
  jcl = new Arc::JobControllerLoader();
}

void JobControllerTest::LoadTest()
{
  jc = jcl->load("", usercfg);
  CPPUNIT_ASSERT(jc == NULL);

  jc = jcl->load("NON-EXISTENT", usercfg);
  CPPUNIT_ASSERT(jc == NULL);

  jc = jcl->load("TEST", usercfg);
  CPPUNIT_ASSERT(jc != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobControllerTest);
