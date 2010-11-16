#include <string>
#include <fstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>
#include <arc/client/JobDescription.h>

#include "../XRSLParser.h"


#define PARSER parser
#define INJOB inJob
#define OUTJOB outJob
#define MESSAGE message

#define UNPARSE_PARSE OUTJOB = PARSER.Parse(PARSER.UnParse(INJOB));

#define PARSE_ASSERT(X) \
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, X);

#define PARSE_ASSERT_EQUAL(X) \
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.X, OUTJOB.X);

#define PARSE_ASSERT_EQUAL2(X, Y) \
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, X, Y);


class XRSLParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(XRSLParserTest);
  CPPUNIT_TEST(TestExecutable);
  CPPUNIT_TEST(TestInputOutputError);
  CPPUNIT_TEST(TestDataStagingDownloadDelete);
  CPPUNIT_TEST(TestDataStagingUploadDelete);
  CPPUNIT_TEST(TestDataStagingCreateDownload);
  CPPUNIT_TEST(TestDataStagingDownloadDownload);
  CPPUNIT_TEST(TestDataStagingUploadDownload);
  CPPUNIT_TEST(TestDataStagingCreateUpload);
  CPPUNIT_TEST(TestDataStagingDownloadUpload);
  CPPUNIT_TEST(TestDataStagingUploadUpload);
  CPPUNIT_TEST(TestNotify);
  CPPUNIT_TEST(TestJoin);
  CPPUNIT_TEST(TestGridTime);
  CPPUNIT_TEST_SUITE_END();

public:
  XRSLParserTest() {}

  void setUp();
  void tearDown();
  void TestExecutable();
  void TestInputOutputError();
  void TestDataStagingDownloadDelete();
  void TestDataStagingUploadDelete();
  void TestDataStagingCreateDownload();
  void TestDataStagingDownloadDownload();
  void TestDataStagingUploadDownload();
  void TestDataStagingCreateUpload();
  void TestDataStagingDownloadUpload();
  void TestDataStagingUploadUpload();
  void TestNotify();
  void TestJoin();
  void TestGridTime();
private:
  Arc::JobDescription INJOB, OUTJOB;
  Arc::XRSLParser PARSER;

  std::string MESSAGE;
};

std::ostream& operator<<(std::ostream& os, const std::list<std::string>& strings) {
  for (std::list<std::string>::const_iterator it = strings.begin(); it != strings.end(); it++) {
    if (it != strings.begin()) {
      os << ", ";
    }
    os << "\"" << *it << "\"";
  }

  return os;
}

void XRSLParserTest::setUp() {
  INJOB.Application.Executable.Name = "executable";
  INJOB.Application.Executable.Argument.push_back("arg1");
  INJOB.Application.Executable.Argument.push_back("arg2");
  INJOB.Application.Executable.Argument.push_back("arg3");

  // Needed by the XRSLParser.
  std::ofstream f("executable", std::ifstream::trunc);
  f << "executable";
  f.close();
}

void XRSLParserTest::tearDown() {
  remove("executable");
}

void XRSLParserTest::TestExecutable() {
  UNPARSE_PARSE;

  PARSE_ASSERT_EQUAL(Application.Executable.Name);
  PARSE_ASSERT_EQUAL(Application.Executable.Argument);
}

void XRSLParserTest::TestInputOutputError() {
  INJOB.Application.Input = "input-file";
  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(INJOB.Application.Input.c_str(), std::ifstream::trunc);
  f << INJOB.Application.Input;
  f.close();

  INJOB.Application.Output = "output-file";
  INJOB.Application.Error = "error-file";

  UNPARSE_PARSE;
  PARSE_ASSERT_EQUAL(Application.Input);
  PARSE_ASSERT_EQUAL(Application.Output);
  PARSE_ASSERT_EQUAL(Application.Error);

  remove(INJOB.Application.Input.c_str());
}

/**
 * In the JobDescription class a file can both have a source and a target
 * location. I.e. a place to be fetched from and a place to put it afterwards.
 * For both source and target it is possible that the file either will be
 * uploaded or downloaded, but it can also be specified that the CE should
 * create/delete the file, in total giving 9 options. Not all of these are
 * supported by GM/can be expressed in XRSL. In the table below the supported
 * cases for XRSL is shown. These are tested in the methods below the table.
 * In addition in the JobDescription the "IsExecutable" and "DownloadToCache"
 * can be specified for a file, these are not fully tested yet.
 *
 *                     T    A    R    G    E    T
 *                ------------------------------------
 *                |  DELETE  |  DOWNLOAD  |  UPLOAD  |
 *  S ------------------------------------------------
 *  O |   CREATE  |   ---    |      X     |    X     |
 *  U ------------------------------------------------
 *  R |  DOWNLOAD |    X     |      X     |    X     |
 *  C ------------------------------------------------
 *  E |   UPLOAD  |    X     |      X     |    X     |
 *    ------------------------------------------------
 *
 **/

 /** 1-Create-Delete:
 * - XRSL does not support this type of specifying data staging elements.
 */

/** 2-Download-Delete */
void XRSLParserTest::TestDataStagingDownloadDelete() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing download-delete data staging type.";

  Arc::FileType file;
  file.Name = "2-Download-Delete";
  Arc::DataSourceType source;
  source.URI = "http://example.com/" + file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = false;
  INJOB.DataStaging.File.push_back(file);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(1, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  */
}

/** 3-Upload-Delete */
void XRSLParserTest::TestDataStagingUploadDelete() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing upload-delete data staging type.";

  Arc::FileType file;
  file.Name = "3-Upload-Delete";
  Arc::DataSourceType source;
  source.URI = file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = false;
  INJOB.DataStaging.File.push_back(file);

  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << file.Name;
  f.close();

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(1, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  */

  remove(file.Name.c_str());
}

/** 4-Create-Download */
void XRSLParserTest::TestDataStagingCreateDownload() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing create-download data staging type.";

  Arc::FileType file;
  file.Name = "4-Create-Download";
  file.KeepData = true;
  INJOB.DataStaging.File.push_back(file);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(1, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  it++;
  */

  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
}

/** 5-Download-Download */
void XRSLParserTest::TestDataStagingDownloadDownload() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing download-download data staging type.";

  Arc::FileType file;
  file.Name = "5-Download-Download";
  Arc::DataSourceType source;
  source.URI = "http://example.com/" + file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = true;
  INJOB.DataStaging.File.push_back(file);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  */

  it++;
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
}

/** 6-Upload-Download */
void XRSLParserTest::TestDataStagingUploadDownload() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing upload-download data staging type.";

  Arc::FileType file;
  file.Name = "6-Upload-Download";
  Arc::DataSourceType source;
  source.URI = file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = true;
  INJOB.DataStaging.File.push_back(file);

  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << file.Name;
  f.close();

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  */

  it++;
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  remove(file.Name.c_str());
}

/** 7-Create-Upload */
void XRSLParserTest::TestDataStagingCreateUpload() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing create-download data staging type.";

  Arc::FileType file;
  file.Name = "7-Create-Upload";
  Arc::DataTargetType target;
  target.URI = "http://example.com/" + file.Name;
  target.Threads = -1;
  file.Target.push_back(target);
  file.KeepData = false;
  INJOB.DataStaging.File.push_back(file);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(1, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  it++;
  */

  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(1, (int)it->Target.size());
  PARSE_ASSERT_EQUAL2(target.URI, it->Target.front().URI);
}

/** 8-Download-Upload */
void XRSLParserTest::TestDataStagingDownloadUpload() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing download-upload data staging type.";

  Arc::FileType file;
  file.Name = "8-Download-Upload";
  Arc::DataSourceType source;
  source.URI = "http://example.com/" + file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  Arc::DataTargetType target;
  target.URI = "http://example.com/" + file.Name;
  target.Threads = -1;
  file.Target.push_back(target);
  file.KeepData = false;
  INJOB.DataStaging.File.push_back(file);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  */

  it++;
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(1, (int)it->Target.size());
  PARSE_ASSERT_EQUAL2(target.URI, it->Target.front().URI);
}

/** 9-Upload-Upload */
void XRSLParserTest::TestDataStagingUploadUpload() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing upload-upload data staging type.";

  Arc::FileType file;
  file.Name = "9-Upload-Upload";
   Arc::DataSourceType source;
  source.URI = file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  Arc::DataTargetType target;
  target.URI = "http://example.com/" + file.Name;
  target.Threads = -1;
  file.Target.push_back(target);
  file.KeepData = false;
  INJOB.DataStaging.File.push_back(file);

  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << file.Name;
  f.close();

  UNPARSE_PARSE;

  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
  */

  it++;
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(1, (int)it->Target.size());
  PARSE_ASSERT_EQUAL2(target.URI, it->Target.front().URI);

  remove(file.Name.c_str());
}

void XRSLParserTest::TestNotify() {
  /**
   * The value of the notify attribute must take the form:
   *   notify = <string> [string] ...
   * with first string being mandatory, and following strings being optional the
   * string should take the form:
   *   [b][q][f][e][d][c] user1@domain1.tld [user2@domain2.tld] ...
   * Thus one email address must be specified for the notify attribute to be
   * valid. States are optional, along with multiple email addresses. Also only
   * the listed states are allowed. If no states are specified the defaults (be)
   * will be used.
   **/

  MESSAGE = "Error parsing the notify attribute.";

  // Test default option.
  std::string xrsl = "&(executable = \"executable\")(notify = \"someone@example.com\")";

  INJOB = PARSER.Parse(xrsl);
  UNPARSE_PARSE;

  PARSE_ASSERT(INJOB);
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(1, (int)INJOB.Application.Notification.size());
  PARSE_ASSERT_EQUAL2(1, (int)OUTJOB.Application.Notification.size());
  PARSE_ASSERT_EQUAL2((std::string)"someone@example.com", INJOB.Application.Notification.front().Email);
  PARSE_ASSERT_EQUAL2((std::string)"someone@example.com", OUTJOB.Application.Notification.front().Email);
  PARSE_ASSERT_EQUAL2(2, (int)INJOB.Application.Notification.front().States.size());
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.Application.Notification.front().States.size());
  PARSE_ASSERT_EQUAL2((std::string)"PREPARING", INJOB.Application.Notification.front().States.front());
  PARSE_ASSERT_EQUAL2((std::string)"FINISHED", INJOB.Application.Notification.front().States.back());
  PARSE_ASSERT_EQUAL2((std::string)"PREPARING", OUTJOB.Application.Notification.front().States.front());
  PARSE_ASSERT_EQUAL2((std::string)"FINISHED", OUTJOB.Application.Notification.front().States.back());

  // Test all flags.
  xrsl = "&(executable = \"executable\")(notify = \"bqfedc someone@example.com\")";

  INJOB = PARSER.Parse(xrsl);
  UNPARSE_PARSE;

  PARSE_ASSERT(INJOB);
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(1, (int)INJOB.Application.Notification.size());
  PARSE_ASSERT_EQUAL2(1, (int)OUTJOB.Application.Notification.size());
  PARSE_ASSERT_EQUAL2((std::string)"someone@example.com", INJOB.Application.Notification.front().Email);
  PARSE_ASSERT_EQUAL2((std::string)"someone@example.com", OUTJOB.Application.Notification.front().Email);
  PARSE_ASSERT_EQUAL2(6, (int)INJOB.Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = INJOB.Application.Notification.front().States.begin();
    PARSE_ASSERT_EQUAL2((std::string)"PREPARING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"INLRMS", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"DELETED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"CANCELING", *it);
  }
  PARSE_ASSERT_EQUAL2(6, (int)OUTJOB.Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = OUTJOB.Application.Notification.front().States.begin();
    PARSE_ASSERT_EQUAL2((std::string)"PREPARING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"INLRMS", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"DELETED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"CANCELING", *it);
  }

  // Test multiple entries and overlapping states.
  xrsl = "&(executable = \"executable\")(notify = \"bqfedc someone@example.com\" \"bqf someone@example.com anotherone@example.com\")";

  INJOB = PARSER.Parse(xrsl);
  UNPARSE_PARSE;

  PARSE_ASSERT(INJOB);
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(2, (int)INJOB.Application.Notification.size());
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.Application.Notification.size());
  PARSE_ASSERT_EQUAL2((std::string)"someone@example.com", INJOB.Application.Notification.front().Email);
  PARSE_ASSERT_EQUAL2((std::string)"someone@example.com", OUTJOB.Application.Notification.front().Email);
  PARSE_ASSERT_EQUAL2((std::string)"anotherone@example.com", INJOB.Application.Notification.back().Email);
  PARSE_ASSERT_EQUAL2((std::string)"anotherone@example.com", OUTJOB.Application.Notification.back().Email);
  PARSE_ASSERT_EQUAL2(6, (int)INJOB.Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = INJOB.Application.Notification.front().States.begin();
    PARSE_ASSERT_EQUAL2((std::string)"PREPARING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"INLRMS", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"DELETED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"CANCELING", *it);
  }
  PARSE_ASSERT_EQUAL2(6, (int)OUTJOB.Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = OUTJOB.Application.Notification.front().States.begin();
    PARSE_ASSERT_EQUAL2((std::string)"PREPARING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"INLRMS", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"DELETED", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"CANCELING", *it);
  }
  PARSE_ASSERT_EQUAL2(3, (int)INJOB.Application.Notification.back().States.size());
  {
    std::list<std::string>::const_iterator it = INJOB.Application.Notification.front().States.begin();
    PARSE_ASSERT_EQUAL2((std::string)"PREPARING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"INLRMS", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHING", *it);
  }
  PARSE_ASSERT_EQUAL2(3, (int)OUTJOB.Application.Notification.back().States.size());
  {
    std::list<std::string>::const_iterator it = OUTJOB.Application.Notification.front().States.begin();
    PARSE_ASSERT_EQUAL2((std::string)"PREPARING", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"INLRMS", *it++);
    PARSE_ASSERT_EQUAL2((std::string)"FINISHING", *it);
  }

  // Test invalid email address.
  xrsl = "&(executable = \"executable\")(notify = \"someoneAexample.com\")";

  INJOB = PARSER.Parse(xrsl);
  PARSE_ASSERT(!INJOB);

  // Test invalid email address with state flags.
  xrsl = "&(executable = \"executable\")(notify = \"bqfecd someoneAexample.com\")";

  INJOB = PARSER.Parse(xrsl);
  PARSE_ASSERT(!INJOB);

  // Test unknown state flags.
  xrsl = "&(executable = \"executable\")(notify = \"xyz someone@example.com\")";

  INJOB = PARSER.Parse(xrsl);
  PARSE_ASSERT(!INJOB);
}

void XRSLParserTest::TestJoin() {
  MESSAGE = "Error parsing the join attribute.";

  std::string xrsl = "&(executable = \"executable\")(stdout = \"output-file\")(join = \"yes\")";

  INJOB = PARSER.Parse(xrsl);
  UNPARSE_PARSE;

  PARSE_ASSERT(INJOB);
  PARSE_ASSERT(OUTJOB);
  // When first parsed the JobDescription attribute Application.Join will be
  // set, while the Application.Error attribute will be left empty.
  PARSE_ASSERT_EQUAL2((std::string)"output-file", INJOB.Application.Output);
  PARSE_ASSERT(INJOB.Application.Error.empty());
  PARSE_ASSERT(INJOB.Application.Join);
  // When the description have been unparsed by the parser the join attribute
  // will no longer be set, instead stdout and stderr will be identical.
  PARSE_ASSERT_EQUAL2((std::string)"output-file", OUTJOB.Application.Output);
  PARSE_ASSERT_EQUAL2((std::string)"output-file", OUTJOB.Application.Error);
  PARSE_ASSERT(!OUTJOB.Application.Join);

  xrsl = "&(executable = \"executable\")(stderr = \"error-file\")(join = \"yes\")";

  INJOB = PARSER.Parse(xrsl);
  UNPARSE_PARSE;

  PARSE_ASSERT(INJOB);
  PARSE_ASSERT(OUTJOB);
  // When first parsed the JobDescription attribute Application.Join will be
  // set, while the Application.Error attribute will be left empty.
  PARSE_ASSERT(INJOB.Application.Output.empty());
  PARSE_ASSERT_EQUAL2((std::string)"error-file", INJOB.Application.Error);
  PARSE_ASSERT(INJOB.Application.Join);
  // When the description have been unparsed by the parser the join attribute
  // will no longer be set, instead stdout and stderr will be identical.
  PARSE_ASSERT_EQUAL2((std::string)"error-file", OUTJOB.Application.Output);
  PARSE_ASSERT_EQUAL2((std::string)"error-file", OUTJOB.Application.Error);
  PARSE_ASSERT(!OUTJOB.Application.Join);

  xrsl = "&(executable = \"executable\")(stdout = \"output-file\")(join = \"no\")";

  INJOB = PARSER.Parse(xrsl);
  UNPARSE_PARSE;

  PARSE_ASSERT(INJOB);
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2((std::string)"output-file", INJOB.Application.Output);
  PARSE_ASSERT_EQUAL2((std::string)"output-file", OUTJOB.Application.Output);
  PARSE_ASSERT(INJOB.Application.Error.empty());
  PARSE_ASSERT(OUTJOB.Application.Error.empty());
  PARSE_ASSERT(!INJOB.Application.Join);
  PARSE_ASSERT(!OUTJOB.Application.Join);
}

void XRSLParserTest::TestGridTime() {
  std::string xrsl = "&(executable=/bin/echo)(gridtime=600s)";
  OUTJOB = PARSER.Parse(xrsl);
  CPPUNIT_ASSERT(OUTJOB);

  CPPUNIT_ASSERT_EQUAL(-1, OUTJOB.Resources.TotalCPUTime.range.min);
  CPPUNIT_ASSERT_EQUAL(600, OUTJOB.Resources.TotalCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL((std::string)"clock rate", OUTJOB.Resources.TotalCPUTime.benchmark.first);
  CPPUNIT_ASSERT_EQUAL(2800., OUTJOB.Resources.TotalCPUTime.benchmark.second);
}

CPPUNIT_TEST_SUITE_REGISTRATION(XRSLParserTest);
