#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <stdlib.h>

#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/client/JobDescriptionParser.h>

class JobDescriptionParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobDescriptionParserTest);
  CPPUNIT_TEST(LoadTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobDescriptionParserTest();
  ~JobDescriptionParserTest() { delete jdpl; }

  void setUp() {}
  void tearDown() { Arc::ThreadInitializer().waitExit(); }

  void LoadTest();

private:
  Arc::JobDescriptionParser *jdp;
  Arc::JobDescriptionParserLoader *jdpl;
};

JobDescriptionParserTest::JobDescriptionParserTest() : jdp(NULL) {
  jdpl = new Arc::JobDescriptionParserLoader();
}

void JobDescriptionParserTest::LoadTest()
{
  jdp = jdpl->load("");
  CPPUNIT_ASSERT(jdp == NULL);

  jdp = jdpl->load("NON-EXISTENT");
  CPPUNIT_ASSERT(jdp == NULL);

  jdp = jdpl->load("TEST");
  CPPUNIT_ASSERT(jdp != NULL);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobDescriptionParserTest);
