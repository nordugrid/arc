#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/Job.h>
#include <arc/client/JobController.h>

#include "JobControllerTestACCControl.h"
#include "JobControllerTestACC.h"

class JobControllerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobControllerTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobControllerTest();
  ~JobControllerTest() { delete jcl; }

  void setUp() {}
  void tearDown() {}

  void LoadTest();

private:
  Arc::JobController *jc;
  Arc::JobControllerLoader *jcl;
  Arc::UserConfig usercfg;
};

JobControllerTest::JobControllerTest() : jc(NULL), usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {
  Arc::SetEnv("ARC_PLUGIN_PATH", ".libs");

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
