#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <cppunit/extensions/HelperMacros.h>
#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include "conf/GMConfig.h"
#include "jobs/JobsList.h"
#include "files/ControlFileHandling.h"

class DTRUploadTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DTRUploadTest);
  CPPUNIT_TEST(TestUploadChecks);
  CPPUNIT_TEST_SUITE_END();
public:
  void setUp() { CPPUNIT_ASSERT(Arc::TmpDirCreate(root)); }
  void tearDown() { Arc::DirDelete(root); }
  void TestUploadChecks();
private:
  std::string root;
};

void DTRUploadTest::TestUploadChecks() {
  using namespace ARex;
  // Missing configuration prevents scheduler/background threads from starting.
  GMConfig config(root + "/nonexistent.conf");
  config.SetControlDir(root + "/control");
  JobsList jobs(config);
  DTRGenerator generator(config, jobs);
  CPPUNIT_ASSERT(!generator);
  for (int test = 0; test < 6; ++test) {
    const std::string id = "12345678900" + Arc::tostring(test);
    const std::string session = root + "/session" + Arc::tostring(test);
    CPPUNIT_ASSERT(Arc::DirCreate(session, 0700));
    CPPUNIT_ASSERT(Arc::DirCreate(job_control_path(config.ControlDir(), id, ""), 0700, true));
    GMJobRef job(new GMJob(id, Arc::User(), session, JOB_STATE_PREPARING));
    JobLocalDescription local;
    local.sessiondir = session;
    CPPUNIT_ASSERT(job_local_write_file(*job, config, local));
    std::list<FileData> files;
    std::string status;
    for (int n = 0; n < (test == 4 ? 0 : 100); ++n) {
      std::string name = "/payload" + Arc::tostring(n);
      files.push_back(FileData(name, test == 5 ? "https://example.invalid/file" : "1"));
      if (test != 1 || n != 99) CPPUNIT_ASSERT(Arc::FileCreate(session + name, "x"));
      status += name + '\n';
    }
    if (test == 2) files.back().lfn = "0"; // last file too large, after 99 successes
    CPPUNIT_ASSERT(job_input_write_file(*job, config, files));
    if (test != 3) CPPUNIT_ASSERT(Arc::FileCreate(job_control_path(config.ControlDir(), id, sfx_inputstatus), status));
    DTRGenerator::checkUploadedFilesResult expected = test == 1 ? DTRGenerator::uploadedFilesMissing :
        test == 2 ? DTRGenerator::uploadedFilesError : DTRGenerator::uploadedFilesSuccess;
    CPPUNIT_ASSERT(generator.checkUploadedFiles(job) == expected);
    std::list<FileData> remaining;
    CPPUNIT_ASSERT(job_input_read_file(id, config, remaining));
    CPPUNIT_ASSERT_EQUAL(std::size_t(test == 5 ? 100 : test == 1 || test == 2 ? 1 : 0), remaining.size());
    if (test == 1 || test == 2) CPPUNIT_ASSERT_EQUAL(std::string("/payload99"), remaining.front().pfn);
    // A second check must not rewrite an unchanged list or lose the result.
    const std::string path = job_control_path(config.ControlDir(), id, sfx_input);
    struct stat before, after;
    CPPUNIT_ASSERT(Arc::FileStat(path, &before, false));
    CPPUNIT_ASSERT(generator.checkUploadedFiles(job) == expected);
    CPPUNIT_ASSERT(Arc::FileStat(path, &after, false));
    CPPUNIT_ASSERT_EQUAL(before.st_ino, after.st_ino);
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(DTRUploadTest);
