#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>
#include <string>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/TargetRetriever.h>

#include "TargetRetrieverTestACCControl.h"
#include "TargetRetrieverTestACC.h"

class TargetRetrieverTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(TargetRetrieverTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  TargetRetrieverTest();
  ~TargetRetrieverTest() { delete trl; }

  void setUp() {}
  void tearDown() {}

  void LoadTest();

private:
  Arc::TargetRetriever *tr;
  Arc::TargetRetrieverLoader *trl;
  Arc::UserConfig usercfg;
};

TargetRetrieverTest::TargetRetrieverTest() : tr(NULL), usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials)) {
  Arc::SetEnv("ARC_PLUGIN_PATH", ".libs");

  trl = new Arc::TargetRetrieverLoader();
}

void TargetRetrieverTest::LoadTest()
{
  tr = trl->load("", usercfg, std::string(), Arc::COMPUTING);
  CPPUNIT_ASSERT(tr == NULL);

  tr = trl->load("NON-EXISTENT", usercfg, std::string(), Arc::COMPUTING);
  CPPUNIT_ASSERT(tr == NULL);

  tr = trl->load("TEST", usercfg, std::string(), Arc::COMPUTING);
  CPPUNIT_ASSERT(tr != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(TargetRetrieverTest);
