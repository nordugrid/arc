// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <unistd.h>
#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Run.h>
#include <arc/User.h>
#include <arc/Utils.h>

class RunTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(RunTest);
  CPPUNIT_TEST(TestRun0);
  CPPUNIT_TEST(TestRun255);
  CPPUNIT_TEST(TestRunMany);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();
  void TestRun0();
  void TestRun255();
  void TestRunMany();

private:
  std::string srcdir;
};


void RunTest::setUp() {
  srcdir = Arc::GetEnv("srcdir");
  if (srcdir.length() == 0)
    srcdir = ".";
}


void RunTest::tearDown() {
  sleep(5); // wait till Run monitoring thread settles down
}

static void initializer_func(void* arg) {
  Arc::UserSwitch u(0,0);
  CPPUNIT_ASSERT(arg == (void*)1);
}

void RunTest::TestRun0() {
  std::string outstr;
  std::string errstr;
  Arc::Run run(srcdir + "/rcode 0");
  run.AssignStdout(outstr);
  run.AssignStderr(errstr);
  run.AssignInitializer(&initializer_func,(void*)1);
  CPPUNIT_ASSERT((bool)run);
  CPPUNIT_ASSERT(run.Start());
  CPPUNIT_ASSERT(run.Wait(10));
  CPPUNIT_ASSERT_EQUAL(0, run.Result());
  CPPUNIT_ASSERT_EQUAL(std::string("STDOUT"), outstr);
  CPPUNIT_ASSERT_EQUAL(std::string("STDERR"), errstr);
}

void RunTest::TestRun255() {
  std::string outstr;
  std::string errstr;
  Arc::Run run(srcdir + "/rcode 255");
  run.AssignStdout(outstr);
  run.AssignStderr(errstr);
  CPPUNIT_ASSERT((bool)run);
  CPPUNIT_ASSERT(run.Start());
  CPPUNIT_ASSERT(run.Wait(10));
  CPPUNIT_ASSERT_EQUAL(255, run.Result());
}

class RunH {
 public:
  unsigned long long int cnt;
  Arc::Run* run;
};

void RunTest::TestRunMany() {
  std::list<RunH> runs;
  for(int n=0;n<5000;++n) {
    //std::cerr<<time(NULL)<<": ";
    for(std::list<RunH>::iterator r = runs.begin();r != runs.end();) {
      //std::cerr<<r->cnt<<"/"<<r->run->getPid()<<" ";
      if(r->run->Running()) { ++(r->cnt); ++r; continue; };
      delete (r->run);
      r = runs.erase(r);
    }
    //std::cerr<<std::endl;
    RunH r;
    r.cnt = 0;
    r.run = new Arc::Run(srcdir + "/rcode 2");
    if(r.run->Start()) runs.push_back(r);
  }
  //std::cerr<<time(NULL)<<": exiting: "<<runs.size()<<std::endl;
  for(std::list<RunH>::iterator r = runs.begin();r != runs.end();++r) {
    CPPUNIT_ASSERT(r->run->Wait(120));
  }
  //std::cerr<<time(NULL)<<": exit"<<std::endl;
}

CPPUNIT_TEST_SUITE_REGISTRATION(RunTest);

