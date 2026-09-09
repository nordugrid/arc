#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <csignal>
#include <sys/resource.h>
#include <cppunit/extensions/HelperMacros.h>
#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include "ControlFileContent.h"

class ControlFileContentTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ControlFileContentTest);
  CPPUNIT_TEST(TestRoundTrip);
  CPPUNIT_TEST(TestRewrite);
  CPPUNIT_TEST(TestWriteFailure);
  CPPUNIT_TEST_SUITE_END();
public:
  void setUp() { CPPUNIT_ASSERT(Arc::TmpDirCreate(root)); }
  void tearDown() { Arc::DirDelete(root); }
  void TestRoundTrip();
  void TestRewrite();
  void TestWriteFailure();
private:
  std::string root;
};

void ControlFileContentTest::TestRoundTrip() {
  const unsigned int sizes[] = { 0, 255, 256, 16383, 16384, 16385, 65537 };
  for(unsigned int size: sizes) {
    ARex::JobLocalDescription job;
    job.jobname.assign(size, 'x');
    job.queue = "queue with spaces";
    job.localid = "123";
    job.sessiondir = "/session/path";
    job.exec.push_back("/bin/echo");
    job.exec.push_back("quotes '\" and backslash \\");
    job.exec.successcode = 7;
    for(unsigned int i = 0; i < 200; ++i) {
      job.activityid.push_back(std::string(100, 'a') + Arc::tostring(i));
      job.tokenclaim["groups"].push_back(Arc::tostring(i));
    }
    CPPUNIT_ASSERT(job.write(root + "/local"));
    ARex::JobLocalDescription result;
    CPPUNIT_ASSERT(result.read(root + "/local"));
    CPPUNIT_ASSERT_EQUAL(job.jobname, result.jobname);
    CPPUNIT_ASSERT_EQUAL(job.queue, result.queue);
    CPPUNIT_ASSERT_EQUAL(job.localid, result.localid);
    CPPUNIT_ASSERT_EQUAL(job.sessiondir, result.sessiondir);
    CPPUNIT_ASSERT(job.exec == result.exec);
    CPPUNIT_ASSERT_EQUAL(job.exec.successcode, result.exec.successcode);
    CPPUNIT_ASSERT(job.activityid == result.activityid);
    CPPUNIT_ASSERT(job.tokenclaim == result.tokenclaim);
    std::string value;
    CPPUNIT_ASSERT(ARex::JobLocalDescription::read_var(root + "/local", "dryrun", value));
    CPPUNIT_ASSERT_EQUAL(std::string("no"), value);
    std::string raw;
    CPPUNIT_ASSERT(Arc::FileRead(root + "/local", raw));
    CPPUNIT_ASSERT(raw.find("queue=queue with spaces\n") != std::string::npos);
    CPPUNIT_ASSERT_EQUAL(std::string("dryrun=no\n"), raw.substr(raw.size()-10));
  }
}

void ControlFileContentTest::TestRewrite() {
  ARex::JobLocalDescription job;
  job.jobname.assign(65537, 'x');
  CPPUNIT_ASSERT(job.write(root + "/local"));
  job.jobname = "short";
  CPPUNIT_ASSERT(job.write(root + "/local"));
  ARex::JobLocalDescription result;
  CPPUNIT_ASSERT(result.read(root + "/local"));
  CPPUNIT_ASSERT_EQUAL(job.jobname, result.jobname);
  std::string raw;
  CPPUNIT_ASSERT(Arc::FileRead(root + "/local", raw));
  CPPUNIT_ASSERT(raw.find("xxxxx") == std::string::npos);
}

void ControlFileContentTest::TestWriteFailure() {
  // Force a failure in the final buffered write, not in open or ftruncate.
  struct LimitGuard {
    struct rlimit saved;
    typedef void (*Handler)(int);
    Handler handler;
    LimitGuard() { getrlimit(RLIMIT_FSIZE, &saved); handler = std::signal(SIGXFSZ, SIG_IGN); }
    ~LimitGuard() { setrlimit(RLIMIT_FSIZE, &saved); std::signal(SIGXFSZ, handler); }
  } guard;
  struct rlimit limit = guard.saved;
  limit.rlim_cur = 128;
  CPPUNIT_ASSERT_EQUAL(0, setrlimit(RLIMIT_FSIZE, &limit));
  ARex::JobLocalDescription job;
  job.jobname.assign(1024, 'x');
  CPPUNIT_ASSERT(!job.write(root + "/local"));
}

CPPUNIT_TEST_SUITE_REGISTRATION(ControlFileContentTest);
