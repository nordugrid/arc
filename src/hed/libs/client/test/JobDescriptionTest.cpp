#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/client/JobDescription.h>

class JobDescriptionTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobDescriptionTest);
  CPPUNIT_TEST(TestAlternative);
  CPPUNIT_TEST_SUITE_END();

public:
  JobDescriptionTest() {};
  void setUp() {}
  void tearDown() {}
  void TestAlternative();
};

void JobDescriptionTest::TestAlternative() {
  {
    Arc::JobDescription j;
    j.Application.Executable.Name = "/bin/exe";
    {
      Arc::JobDescription altJ;
      altJ.Application.Executable.Name = "/bin/alt1exe";
      j.AddAlternative(altJ);
    }

    {
      Arc::JobDescription altJ;
      altJ.Application.Executable.Name = "/bin/alt2exe";

      {
        Arc::JobDescription altaltJ;
        altaltJ.Application.Executable.Name = "/bin/alt3exe";
        altJ.AddAlternative(altaltJ);
      }

      j.AddAlternative(altJ);
    }

    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", j.Application.Executable.Name);
    CPPUNIT_ASSERT(j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/alt1exe", j.Application.Executable.Name);
    CPPUNIT_ASSERT(j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/alt2exe", j.Application.Executable.Name);
    CPPUNIT_ASSERT(j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/alt3exe", j.Application.Executable.Name);
    CPPUNIT_ASSERT(!j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    j.UseOriginal();
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", j.Application.Executable.Name);
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobDescriptionTest);
