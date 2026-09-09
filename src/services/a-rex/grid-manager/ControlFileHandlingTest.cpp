#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <cppunit/extensions/HelperMacros.h>
#include <arc/FileUtils.h>
#include "conf/GMConfig.h"
#include "files/ControlFileHandling.h"
#include "jobs/JobDescriptionHandler.h"

class ControlFileHandlingTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ControlFileHandlingTest);
  CPPUNIT_TEST(TestStateDirectories);
  CPPUNIT_TEST(TestMissingState);
  CPPUNIT_TEST(TestInvalidState);
  CPPUNIT_TEST(TestUnreadableState);
  CPPUNIT_TEST(TestStateLinks);
  CPPUNIT_TEST(TestFailureFields);
  CPPUNIT_TEST(TestFinalCleanup);
  CPPUNIT_TEST(TestStatusUpdates);
  CPPUNIT_TEST(TestLocalID);
  CPPUNIT_TEST_SUITE_END();
public:
  void setUp() {
    CPPUNIT_ASSERT(Arc::TmpDirCreate(root));
    config.SetControlDir(root);
    for (const char* dir : {"processing", "accepting", "restarting", "finished"})
      CPPUNIT_ASSERT(Arc::DirCreate(root + "/" + dir, 0700));
  }
  void tearDown() { Arc::DirDelete(root); }
  void TestStateDirectories();
  void TestMissingState();
  void TestInvalidState();
  void TestUnreadableState();
  void TestStateLinks();
  void TestFailureFields();
  void TestFinalCleanup();
  void TestStatusUpdates();
  void TestLocalID();
private:
  std::string root;
  ARex::GMConfig config;
};

void ControlFileHandlingTest::TestStateDirectories() {
  for (const char* dir : {"processing", "accepting", "restarting", "finished"}) {
    std::string path = root + "/" + dir + "/123.status";
    CPPUNIT_ASSERT(Arc::FileCreate(path, "PENDING:FINISHED\n"));
    bool pending = false;
    CPPUNIT_ASSERT(ARex::job_state_read_file("123", config, pending) == ARex::JOB_STATE_FINISHED);
    CPPUNIT_ASSERT(pending);
    CPPUNIT_ASSERT(Arc::FileCreate(path, "FINISHED\n"));
    CPPUNIT_ASSERT(ARex::job_state_read_file("123", config, pending) == ARex::JOB_STATE_FINISHED);
    CPPUNIT_ASSERT(!pending);
    CPPUNIT_ASSERT(Arc::FileDelete(path));
  }
}

void ControlFileHandlingTest::TestMissingState() {
  CPPUNIT_ASSERT(ARex::job_state_read_file("123", config) == ARex::JOB_STATE_DELETED);
  CPPUNIT_ASSERT(Arc::FileCreate(root + "/not-a-directory", ""));
  config.SetControlDir(root + "/not-a-directory");
  CPPUNIT_ASSERT(ARex::job_state_read_file("123", config) == ARex::JOB_STATE_DELETED);
}

void ControlFileHandlingTest::TestInvalidState() {
  CPPUNIT_ASSERT(Arc::FileCreate(root + "/finished/123.status", "FINISHED"));
  for (const char* content : {"", "invalid", "PENDING:invalid"}) {
    CPPUNIT_ASSERT(Arc::FileCreate(root + "/processing/123.status", content));
    CPPUNIT_ASSERT(ARex::job_state_read_file("123", config) == ARex::JOB_STATE_UNDEFINED);
  }
}

void ControlFileHandlingTest::TestUnreadableState() {
  // Run as an ordinary user to exercise EACCES rather than root's DAC override.
  if (getuid() == 0) return;
  std::string path = root + "/processing/123.status";
  CPPUNIT_ASSERT(Arc::FileCreate(path, "INLRMS"));
  CPPUNIT_ASSERT_EQUAL(0, chmod(path.c_str(), 0000));
  CPPUNIT_ASSERT(Arc::FileCreate(root + "/finished/123.status", "FINISHED"));
  CPPUNIT_ASSERT(ARex::job_state_read_file("123", config) == ARex::JOB_STATE_UNDEFINED);
}

void ControlFileHandlingTest::TestStateLinks() {
  std::string path = root + "/processing/123.status";
  CPPUNIT_ASSERT_EQUAL(0, symlink("missing", path.c_str()));
  CPPUNIT_ASSERT(ARex::job_state_read_file("123", config) == ARex::JOB_STATE_DELETED);
  CPPUNIT_ASSERT(Arc::FileCreate(root + "/finished/123.status", "FINISHED"));
  CPPUNIT_ASSERT(ARex::job_state_read_file("123", config) == ARex::JOB_STATE_FINISHED);
  CPPUNIT_ASSERT(Arc::FileDelete(path));
  CPPUNIT_ASSERT_EQUAL(0, symlink("../finished/123.status", path.c_str()));
  CPPUNIT_ASSERT(ARex::job_state_read_file("123", config) == ARex::JOB_STATE_FINISHED);
}

void ControlFileHandlingTest::TestFailureFields() {
  std::string path = ARex::job_control_path(root, "123456789001", "local");
  CPPUNIT_ASSERT(Arc::DirCreate(ARex::job_control_path(root, "123456789001", ""), 0700, true));
  struct Case { const char* content; const char* state; const char* cause; };
  const Case cases[] = {
    {"", "", ""},
    {"failedstate=PREPARING\n", "PREPARING", ""},
    {"failedcause=client\n", "", "client"},
    {"failedcause=client\nfailedstate=PREPARING", "PREPARING", "client"},
    {"failedstate=\nfailedstate=PREPARING\nfailedstate=FINISHING\nfailedcause=internal\n", "PREPARING", "internal"}
  };
  for (const Case& test : cases) {
    CPPUNIT_ASSERT(Arc::FileCreate(path, std::string(65536, 'x') + "=ignored\n" + test.content));
    std::string state = "old", cause = "old";
    CPPUNIT_ASSERT(ARex::job_local_read_failed("123456789001", config, state, cause));
    CPPUNIT_ASSERT_EQUAL(std::string(test.state), state);
    CPPUNIT_ASSERT_EQUAL(std::string(test.cause), cause);
  }
  CPPUNIT_ASSERT(Arc::FileDelete(path));
  std::string state = "old", cause = "old";
  CPPUNIT_ASSERT(ARex::job_local_read_failed("123456789001", config, state, cause));
  CPPUNIT_ASSERT(state.empty() && cause.empty());
}

void ControlFileHandlingTest::TestFinalCleanup() {
  const std::string id = "123456789001";
  const std::string neighbour = "123456789002";
  const std::string path = ARex::job_control_path(root, id, "");
  const std::string retained = ARex::job_control_path(root, neighbour, "local");
  CPPUNIT_ASSERT(Arc::DirCreate(path, 0700, true));
  CPPUNIT_ASSERT(Arc::DirCreate(ARex::job_control_path(root, neighbour, ""), 0700, true));
  CPPUNIT_ASSERT(Arc::FileCreate(retained, "keep"));
  for (const char* suffix : {"proxy_tmp", "lrms_done", "lrms_job", "local", "description", "input", "output"})
    CPPUNIT_ASSERT(Arc::FileCreate(path + suffix, "fixture"));
  for (const char* dir : {"accepting", "processing", "restarting", "finished"})
    CPPUNIT_ASSERT(Arc::FileCreate(root + "/" + dir + "/" + id + ".status", "FINISHED"));
  const std::string session = root + "/session";
  CPPUNIT_ASSERT(Arc::DirCreate(session, 0700));
  CPPUNIT_ASSERT(Arc::FileCreate(session + "/payload", "fixture"));
  CPPUNIT_ASSERT(Arc::FileCreate(session + ".diag", "fixture"));
  CPPUNIT_ASSERT(Arc::FileCreate(session + ".comment", "fixture"));
  ARex::GMJob job(id, Arc::User(), session, ARex::JOB_STATE_FINISHED);
  CPPUNIT_ASSERT(ARex::job_clean_final(job, config));
  struct stat st;
  CPPUNIT_ASSERT(!Arc::FileStat(path, &st, false));
  CPPUNIT_ASSERT(!Arc::FileStat(session, &st, false));
  CPPUNIT_ASSERT(!Arc::FileStat(session + ".diag", &st, false));
  CPPUNIT_ASSERT(!Arc::FileStat(session + ".comment", &st, false));
  for (const char* dir : {"accepting", "processing", "restarting", "finished"})
    CPPUNIT_ASSERT(!Arc::FileStat(root + "/" + dir + "/" + id + ".status", &st, false));
  std::string value;
  CPPUNIT_ASSERT(Arc::FileRead(retained, value));
  CPPUNIT_ASSERT_EQUAL(std::string("keep"), value);
  CPPUNIT_ASSERT(ARex::job_clean_final(job, config)); // repeated cleanup stays safe
}

void ControlFileHandlingTest::TestStatusUpdates() {
  const std::string id = "123456789001";
  const std::string path = ARex::job_control_path(root, id, "");
  CPPUNIT_ASSERT(Arc::DirCreate(path, 0700, true));
  ARex::GMJob job(id, Arc::User(), "", ARex::JOB_STATE_PREPARING);
  for (bool input : {true, false}) {
    const std::string status = path + (input ? "input_status" : "output_status");
    std::string expected;
    for (const char* name : {"/first file", "/second", ""}) {
      ARex::FileData file(name, "https://example.org/result");
      std::ostringstream line;
      if (input) line << name << '\n';
      else line << file << '\n';
      expected += line.str();
      // Existing permissive files must also be replaced with private files.
      if (name != std::string("/first file"))
        CPPUNIT_ASSERT_EQUAL(0, chmod(status.c_str(), 0666));
      const mode_t old_umask = umask(input ? 0077 : 0777);
      const bool updated = input ? ARex::job_input_status_add_file(job, config, name)
                                 : ARex::job_output_status_add_file(job, config, file);
      umask(old_umask);
      CPPUNIT_ASSERT(updated);
      std::string content;
      CPPUNIT_ASSERT(Arc::FileRead(status, content));
      CPPUNIT_ASSERT_EQUAL(expected, content);
      struct stat st;
      CPPUNIT_ASSERT(Arc::FileStat(status, &st, false));
      CPPUNIT_ASSERT_EQUAL(mode_t(0600), mode_t(st.st_mode & 07777));
      CPPUNIT_ASSERT_EQUAL(uid_t(job.get_user().get_uid()), st.st_uid);
      CPPUNIT_ASSERT_EQUAL(gid_t(job.get_user().get_gid()), st.st_gid);
    }
    if (getuid() != 0) {
      CPPUNIT_ASSERT_EQUAL(0, chmod(status.c_str(), 0000));
      const bool updated = input ? ARex::job_input_status_add_file(job, config, "/failed")
                                 : ARex::job_output_status_add_file(job, config, ARex::FileData("/failed", ""));
      CPPUNIT_ASSERT(!updated);
      CPPUNIT_ASSERT_EQUAL(0, chmod(status.c_str(), 0600));
      std::string content;
      CPPUNIT_ASSERT(Arc::FileRead(status, content));
      CPPUNIT_ASSERT_EQUAL(expected, content);
      if (input) {
        // A failed update must release its lock for the next writer.
        CPPUNIT_ASSERT(ARex::job_input_status_add_file(job, config, "/retry"));
      }
    }
  }
}

void ControlFileHandlingTest::TestLocalID() {
  const std::string id = "123456789001";
  const std::string path = ARex::job_control_path(root, id, "grami");
  CPPUNIT_ASSERT(Arc::DirCreate(ARex::job_control_path(root, id, ""), 0700, true));
  ARex::JobDescriptionHandler handler(config);
  CPPUNIT_ASSERT(handler.get_local_id(id).empty()); // missing file
  struct Case { const char* content; const char* expected; };
  const Case cases[] = {
    {"", ""},
    {"joboption_other=42\n", ""},
    {"joboption_jobid='42'\njoboption_jobid='99'\n", "42"},
    {"joboption_jobid=\njoboption_jobid=99\n", ""},
    {" joboption_jobid=99\njoboption_jobid=42", "42"},
    {"joboption_jobid='42.1'", "42.1"}
  };
  for (const Case& test : cases) {
    CPPUNIT_ASSERT(Arc::FileCreate(path, test.content));
    CPPUNIT_ASSERT_EQUAL(std::string(test.expected), handler.get_local_id(id));
  }
  const std::string padding(1024*1024, 'x');
  for (bool first : {true, false}) {
    const std::string field = "joboption_jobid='12345'\n";
    CPPUNIT_ASSERT(Arc::FileCreate(path, first ? field + padding : padding + "\n" + field));
    CPPUNIT_ASSERT_EQUAL(std::string("12345"), handler.get_local_id(id));
  }
  if (getuid() != 0) {
    CPPUNIT_ASSERT_EQUAL(0, chmod(path.c_str(), 0000));
    CPPUNIT_ASSERT(handler.get_local_id(id).empty());
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(ControlFileHandlingTest);
