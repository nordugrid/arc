#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobDescription.h>

class JobDescriptionTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobDescriptionTest);
  CPPUNIT_TEST(TestAlternative);
  CPPUNIT_TEST(PrepareTest);
  CPPUNIT_TEST_SUITE_END();

public:
  JobDescriptionTest() {};
  void setUp() {}
  void tearDown() {}
  void TestAlternative();
  void PrepareTest();
};

void JobDescriptionTest::TestAlternative() {
  {
    Arc::JobDescription j;
    j.Application.Executable.Path = "/bin/exe";
    {
      Arc::JobDescription altJ;
      altJ.Application.Executable.Path = "/bin/alt1exe";
      j.AddAlternative(altJ);
    }

    {
      Arc::JobDescription altJ;
      altJ.Application.Executable.Path = "/bin/alt2exe";

      {
        Arc::JobDescription altaltJ;
        altaltJ.Application.Executable.Path = "/bin/alt3exe";
        altJ.AddAlternative(altaltJ);
      }

      j.AddAlternative(altJ);
    }

    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", j.Application.Executable.Path);
    CPPUNIT_ASSERT(j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/alt1exe", j.Application.Executable.Path);
    CPPUNIT_ASSERT(j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/alt2exe", j.Application.Executable.Path);
    CPPUNIT_ASSERT(j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/alt3exe", j.Application.Executable.Path);
    CPPUNIT_ASSERT(!j.UseAlternative());
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
    j.UseOriginal();
    CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", j.Application.Executable.Path);
    CPPUNIT_ASSERT_EQUAL(3, (int)j.GetAlternatives().size());
  }
}

void JobDescriptionTest::PrepareTest()
{
  {
    Arc::JobDescription jd;
    jd.Application.Executable.Path = "/hello/world";
    CPPUNIT_ASSERT(jd.Prepare(Arc::ExecutionTarget()));
  }

  {
    const std::string exec = "PrepareJobDescriptionTest-executable";
    remove(exec.c_str());
    Arc::JobDescription jd;
    jd.Application.Executable.Path = exec;
    CPPUNIT_ASSERT(!jd.Prepare(Arc::ExecutionTarget()));
    std::ofstream f(exec.c_str(), std::ifstream::trunc);
    f << exec;
    f.close();
    CPPUNIT_ASSERT(jd.Prepare(Arc::ExecutionTarget()));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.DataStaging.InputFiles.size());
    CPPUNIT_ASSERT_EQUAL(jd.Application.Executable.Path, jd.DataStaging.InputFiles.front().Name);
    remove(exec.c_str());
  }

  {
    const std::string input = "PrepareJobDescriptionTest-input";
    remove(input.c_str());
    Arc::JobDescription jd;
    jd.Application.Input = input;
    CPPUNIT_ASSERT(!jd.Prepare(Arc::ExecutionTarget()));
    std::ofstream f(input.c_str(), std::ifstream::trunc);
    f << input;
    f.close();
    CPPUNIT_ASSERT(jd.Prepare(Arc::ExecutionTarget()));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.DataStaging.InputFiles.size());
    CPPUNIT_ASSERT_EQUAL(jd.Application.Input, jd.DataStaging.InputFiles.front().Name);
    remove(input.c_str());
  }

  {
    Arc::JobDescription jd;
    jd.Application.Output = "PrepareJobDescriptionTest-output";
    CPPUNIT_ASSERT(jd.Prepare(Arc::ExecutionTarget()));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.DataStaging.OutputFiles.size());
    CPPUNIT_ASSERT_EQUAL(jd.Application.Output, jd.DataStaging.OutputFiles.front().Name);
  }

  {
    Arc::JobDescription jd;
    jd.Application.Error = "PrepareJobDescriptionTest-error";
    CPPUNIT_ASSERT(jd.Prepare(Arc::ExecutionTarget()));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.DataStaging.OutputFiles.size());
    CPPUNIT_ASSERT_EQUAL(jd.Application.Error, jd.DataStaging.OutputFiles.front().Name);
  }

  {
    Arc::JobDescription jd;
    jd.Application.LogDir = "PrepareJobDescriptionTest-logdir";
    CPPUNIT_ASSERT(jd.Prepare(Arc::ExecutionTarget()));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.DataStaging.OutputFiles.size());
    CPPUNIT_ASSERT_EQUAL(jd.Application.LogDir, jd.DataStaging.OutputFiles.front().Name);
  }

  {
    Arc::JobDescription jd;
    Arc::ExecutionTarget et;
    et.ApplicationEnvironments->push_back(Arc::ApplicationEnvironment("SOFTWARE/HELLOWORLD-1.0.0"));
    jd.Resources.RunTimeEnvironment.add(Arc::Software("SOFTWARE/HELLOWORLD-1.0.0"), Arc::Software::GREATERTHAN);
    CPPUNIT_ASSERT(!jd.Prepare(et));
    et.ApplicationEnvironments->push_back(Arc::ApplicationEnvironment("SOFTWARE/HELLOWORLD-2.0.0"));
    CPPUNIT_ASSERT(jd.Prepare(et));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.Resources.RunTimeEnvironment.getSoftwareList().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"SOFTWARE/HELLOWORLD", jd.Resources.RunTimeEnvironment.getSoftwareList().front().getName());
    CPPUNIT_ASSERT_EQUAL((std::string)"2.0.0", jd.Resources.RunTimeEnvironment.getSoftwareList().front().getVersion());
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.Resources.RunTimeEnvironment.getComparisonOperatorList().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"==", Arc::Software::toString(jd.Resources.RunTimeEnvironment.getComparisonOperatorList().front()));
  }

  {
    Arc::JobDescription jd;
    Arc::ExecutionTarget et;
    et.ComputingEndpoint->Implementation = Arc::Software("MIDDLEWARE/ABC-1.2.3");
    jd.Resources.CEType.add(Arc::Software("MIDDLEWARE/ABC-2.0.0"), Arc::Software::GREATERTHANOREQUAL);
    CPPUNIT_ASSERT(!jd.Prepare(et));
    jd.Resources.CEType.clear();
    jd.Resources.CEType.add(Arc::Software("MIDDLEWARE/ABC-1.2.3"), Arc::Software::GREATERTHANOREQUAL);
    CPPUNIT_ASSERT(jd.Prepare(et));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.Resources.CEType.getSoftwareList().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"MIDDLEWARE/ABC", jd.Resources.CEType.getSoftwareList().front().getName());
    CPPUNIT_ASSERT_EQUAL((std::string)"1.2.3", jd.Resources.CEType.getSoftwareList().front().getVersion());
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.Resources.CEType.getComparisonOperatorList().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"==", Arc::Software::toString(jd.Resources.CEType.getComparisonOperatorList().front()));
  }

  {
    Arc::JobDescription jd;
    Arc::ExecutionTarget et;
    et.ExecutionEnvironment->OperatingSystem = Arc::Software("OPERATINGSYSTEM/COW-2.2.2");
    jd.Resources.OperatingSystem.add(Arc::Software("OPERATINGSYSTEM/COW-2.0.0"), Arc::Software::LESSTHAN);
    CPPUNIT_ASSERT(!jd.Prepare(et));
    jd.Resources.OperatingSystem.clear();
    jd.Resources.OperatingSystem.add(Arc::Software("OPERATINGSYSTEM/COW-3.0.0"), Arc::Software::LESSTHAN);
    CPPUNIT_ASSERT(jd.Prepare(et));
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.Resources.OperatingSystem.getSoftwareList().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"OPERATINGSYSTEM/COW", jd.Resources.OperatingSystem.getSoftwareList().front().getName());
    CPPUNIT_ASSERT_EQUAL((std::string)"2.2.2", jd.Resources.OperatingSystem.getSoftwareList().front().getVersion());
    CPPUNIT_ASSERT_EQUAL(1, (int)jd.Resources.OperatingSystem.getComparisonOperatorList().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"==", Arc::Software::toString(jd.Resources.OperatingSystem.getComparisonOperatorList().front()));
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobDescriptionTest);
