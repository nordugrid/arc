#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/compute/JobDescriptionParserPlugin.h>

class JobDescriptionParserPluginTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobDescriptionParserPluginTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobDescriptionParserPluginTest();
  ~JobDescriptionParserPluginTest() { delete jdpl; }

  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }

  void LoadTest();

private:
  Arc::JobDescriptionParserPlugin *jdp;
  Arc::JobDescriptionParserPluginLoader *jdpl;
};

JobDescriptionParserPluginTest::JobDescriptionParserPluginTest() : jdp(NULL) {
  jdpl = new Arc::JobDescriptionParserPluginLoader();
}

void JobDescriptionParserPluginTest::LoadTest()
{
  jdp = jdpl->load("");
  CPPUNIT_ASSERT(jdp == NULL);

  jdp = jdpl->load("NON-EXISTENT");
  CPPUNIT_ASSERT(jdp == NULL);

  jdp = jdpl->load("TEST");
  CPPUNIT_ASSERT(jdp != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobDescriptionParserPluginTest);
