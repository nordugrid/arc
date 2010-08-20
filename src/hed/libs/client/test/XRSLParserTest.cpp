#include <string>
#include <fstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>

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
  CPPUNIT_TEST_SUITE_END();

public:
  XRSLParserTest()
    : logger(Arc::Logger::getRootLogger(), "XRSLParserTest"),
      logcerr(std::cout)
    {}
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

private:
  Arc::JobDescription INJOB, OUTJOB;
  Arc::XRSLParser PARSER;

  std::string MESSAGE;

  Arc::LogStream logcerr;
  Arc::Logger logger;
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

  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);
}

void XRSLParserTest::tearDown() {
}

void XRSLParserTest::TestExecutable() {
  UNPARSE_PARSE;

  PARSE_ASSERT_EQUAL(Application.Executable.Name);
  PARSE_ASSERT_EQUAL(Application.Executable.Argument);
}

void XRSLParserTest::TestInputOutputError() {
  INJOB.Application.Input = "input-file";
  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f("input-file", std::ifstream::trunc);
  f << "input-file";
  f.close();

  INJOB.Application.Output = "output-file";
  INJOB.Application.Error = "error-file";

  UNPARSE_PARSE;
  PARSE_ASSERT_EQUAL(Application.Input);
  PARSE_ASSERT_EQUAL(Application.Output);
  PARSE_ASSERT_EQUAL(Application.Error);
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
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
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
  std::ofstream f("3-Upload-Delete", std::ifstream::trunc);
  f << "3-Upload-Delete";
  f.close();

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
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
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
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
  PARSE_ASSERT_EQUAL2(3, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

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
  f << "6-Upload-Download";
  f.close();

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(3, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());
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
  PARSE_ASSERT_EQUAL2(2, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
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
  PARSE_ASSERT_EQUAL2(3, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

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
  f << "9-Upload-Upload";
  f.close();

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT_EQUAL2(3, (int)OUTJOB.DataStaging.File.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOB.DataStaging.File.begin();
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(source.URI, it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2((std::string)"executable", it->Name);
  PARSE_ASSERT_EQUAL2(false, it->KeepData);
  PARSE_ASSERT_EQUAL2(1, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(Arc::URL("executable"), it->Source.front().URI);
  PARSE_ASSERT_EQUAL2(0, (int)it->Target.size());

  it++;
  PARSE_ASSERT_EQUAL2(file.Name, it->Name);
  PARSE_ASSERT_EQUAL2(file.KeepData, it->KeepData);
  PARSE_ASSERT_EQUAL2(0, (int)it->Source.size());
  PARSE_ASSERT_EQUAL2(1, (int)it->Target.size());
  PARSE_ASSERT_EQUAL2(target.URI, it->Target.front().URI);
}

CPPUNIT_TEST_SUITE_REGISTRATION(XRSLParserTest);
