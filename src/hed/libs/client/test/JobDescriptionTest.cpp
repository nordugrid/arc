#include <iostream>
#include <string>
#include <fstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/client/JobDescription.h>

#define ARC_ASSERT(X) CPPUNIT_ASSERT_MESSAGE("ARCJSDL parsing issue", X);
#define XRSL_ASSERT(X) CPPUNIT_ASSERT_MESSAGE("XRSL parsing issue", X);
#define JDL_ASSERT(X) CPPUNIT_ASSERT_MESSAGE("JDL parsing issue", X);

#define PARSE_ASSERT(X, JOB, ARC, XRSL, JDL) \
  ARC_ASSERT(ARC.X == JOB.X); \
  XRSL_ASSERT(XRSL.X == JOB.X); \
  JDL_ASSERT(JDL.X == JOB.X);

#define ARC_ASSERT_EQUAL(X, Y) CPPUNIT_ASSERT_EQUAL_MESSAGE("ARCJSDL parsing issue", X, Y);
#define XRSL_ASSERT_EQUAL(X, Y) CPPUNIT_ASSERT_EQUAL_MESSAGE("XRSL parsing issue", X, Y);
#define JDL_ASSERT_EQUAL(X, Y) CPPUNIT_ASSERT_EQUAL_MESSAGE("JDL parsing issue", X, Y);

#define PARSE_ASSERT_EQUAL(X, JOB, ARC, XRSL, JDL) \
  ARC_ASSERT_EQUAL(JOB.X, ARC.X); \
  XRSL_ASSERT_EQUAL(JOB.X, XRSL.X); \
  JDL_ASSERT_EQUAL(JOB.X, JDL.X);

#define DATATYPE_ASSERT() \


class JobDescriptionTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JobDescriptionTest);
  CPPUNIT_TEST(TestExecutable);
  CPPUNIT_TEST(TestInputOutputError);
  CPPUNIT_TEST(TestDataStagingCreateDelete);
  CPPUNIT_TEST(TestDataStagingDownloadDelete);
  CPPUNIT_TEST(TestDataStagingUploadDelete);
  CPPUNIT_TEST(TestDataStagingCreateDownload);
  CPPUNIT_TEST(TestDataStagingDownloadDownload);
  CPPUNIT_TEST(TestDataStagingUploadDownload);
  CPPUNIT_TEST(TestDataStagingCreateUpload);
  CPPUNIT_TEST(TestDataStagingDownloadUpload);
  CPPUNIT_TEST(TestDataStagingUploadUpload);
  CPPUNIT_TEST(TestPOSIXCompliance);
  CPPUNIT_TEST(TestHPCCompliance);
  CPPUNIT_TEST_SUITE_END();

public:
  JobDescriptionTest()
    : logger(Arc::Logger::getRootLogger(), "JobDescriptionTest"),
      logcerr(std::cout)
    {}
  void setUp();
  void tearDown();
  void TestExecutable();
  void TestInputOutputError();
  void TestDataStagingCreateDelete();
  void TestDataStagingDownloadDelete();
  void TestDataStagingUploadDelete();
  void TestDataStagingCreateDownload();
  void TestDataStagingDownloadDownload();
  void TestDataStagingUploadDownload();
  void TestDataStagingCreateUpload();
  void TestDataStagingDownloadUpload();
  void TestDataStagingUploadUpload();
  void TestPOSIXCompliance();
  void TestHPCCompliance();

private:
  Arc::JobDescription job;
  Arc::JobDescription arcJob;
  Arc::JobDescription xrslJob;
  Arc::JobDescription jdlJob;

  Arc::LogStream logcerr;
  Arc::Logger logger;
};

void JobDescriptionTest::setUp() {
  job.Application.Executable.Name = "executable";
  job.Application.Executable.Argument.push_back("arg1");
  job.Application.Executable.Argument.push_back("arg2");
  job.Application.Executable.Argument.push_back("arg3");

  // Needed by the XRSLParser.
  std::ofstream f("executable", std::ifstream::trunc);
  f << "executable";
  f.close();
  
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);
}

void JobDescriptionTest::tearDown() {
}

void JobDescriptionTest::TestExecutable() {
  arcJob.Parse(job.UnParse("ARCJSDL"));
  xrslJob.Parse(job.UnParse("XRSL"));
  jdlJob.Parse(job.UnParse("JDL"));

  PARSE_ASSERT(Application.Executable.Name, job, arcJob, xrslJob, jdlJob);
  PARSE_ASSERT(Application.Executable.Argument, job, arcJob, xrslJob, jdlJob);
}

void JobDescriptionTest::TestInputOutputError() {
  job.Application.Input = "input-file";
  // Needed by the XRSLParser.
  std::ofstream f("input-file", std::ifstream::trunc);
  f << "input-file";
  f.close();

  job.Application.Output = "output-file";
  job.Application.Error = "error-file";

  arcJob.Parse(job.UnParse("ARCJSDL"));
  xrslJob.Parse(job.UnParse("XRSL"));
  jdlJob.Parse(job.UnParse("JDL"));

  PARSE_ASSERT(Application.Input, job, arcJob, xrslJob, jdlJob);
  PARSE_ASSERT(Application.Output, job, arcJob, xrslJob, jdlJob);
  PARSE_ASSERT(Application.Error, job, arcJob, xrslJob, jdlJob);
}

 /** 1-Create-Delete:
 * - XRSL and JDL does not support this type of specifying data staging elements.
 */
void JobDescriptionTest::TestDataStagingCreateDelete() {
      job.DataStaging.File.clear();
    
  Arc::FileType file;
  file.Name = "1-Create-Delete";
  file.KeepData = false;
  file.IsExecutable = false;
  file.DownloadToCache = false;
  job.DataStaging.File.push_back(file);

  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(job.DataStaging.File.size() == 1 && arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);

  ARC_ASSERT(job.DataStaging.File.front().IsExecutable == arcJob.DataStaging.File.front().IsExecutable);
  job.DataStaging.File.pop_back(); file.IsExecutable = true; job.DataStaging.File.push_back(file); arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(job.DataStaging.File.front().IsExecutable == arcJob.DataStaging.File.front().IsExecutable);

  ARC_ASSERT(job.DataStaging.File.front().DownloadToCache == arcJob.DataStaging.File.front().DownloadToCache);
  job.DataStaging.File.pop_back(); file.DownloadToCache = true; job.DataStaging.File.push_back(file); arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(job.DataStaging.File.front().DownloadToCache == arcJob.DataStaging.File.front().DownloadToCache);
}

/** 2-Download-Delete */
void JobDescriptionTest::TestDataStagingDownloadDelete() {
  job.DataStaging.File.clear();

  Arc::FileType file;
  file.Name = "2-Download-Delete";
  Arc::DataSourceType source;
  source.URI = "http://example.com/" + file.Name;
  CPPUNIT_ASSERT(source.URI);
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = false;
  file.IsExecutable = false;
  file.DownloadToCache = false;
  job.DataStaging.File.push_back(file);

  CPPUNIT_ASSERT(job.DataStaging.File.size() == 1);
  CPPUNIT_ASSERT(job.DataStaging.File.front().Source.size() == 1 && job.DataStaging.File.front().Source.front().URI);

  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().Source.size() == 1 && arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Source.front().URI == arcJob.DataStaging.File.front().Source.front().URI);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.front().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.front().Source.size() == 1 && xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(job.DataStaging.File.front().Source.front().URI == xrslJob.DataStaging.File.front().Source.front().URI);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.front().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.front().Source.size() == 1 && jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Source.front().URI == jdlJob.DataStaging.File.front().Source.front().URI);
}

/** 3-Upload-Delete */
void JobDescriptionTest::TestDataStagingUploadDelete() {
  job.DataStaging.File.clear();

  Arc::FileType file;
  file.Name = "3-Upload-Delete";
  Arc::DataSourceType source;
  source.URI = file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = false;
  job.DataStaging.File.push_back(file);

  // Needed by the XRSLParser.
  std::ofstream f("3-Upload-Delete", std::ifstream::trunc);
  f << "3-Upload-Delete";
  f.close();

  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().Source.size() == 1 && arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Source.front().URI == arcJob.DataStaging.File.front().Source.front().URI);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.front().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.front().Source.size() == 1 && xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(job.DataStaging.File.front().Source.front().URI == xrslJob.DataStaging.File.front().Source.front().URI);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.front().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.front().Source.size() == 1 && jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Source.front().URI == jdlJob.DataStaging.File.front().Source.front().URI);
}

/** 4-Create-Download */
void JobDescriptionTest::TestDataStagingCreateDownload() {
  job.DataStaging.File.clear();

  Arc::FileType file;
  file.Name = "4-Create-Download";
  file.KeepData = true;
  job.DataStaging.File.push_back(file);

  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().KeepData);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.back().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.back().KeepData);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.back().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.back().KeepData);
}

/** 5-Download-Download */
void JobDescriptionTest::TestDataStagingDownloadDownload() {
  job.DataStaging.File.clear();

  Arc::FileType file;
  file.Name = "5-Download-Download";
  Arc::DataSourceType source;
  source.URI = "http://example.com/" + file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = true;
  job.DataStaging.File.push_back(file);

  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().Source.size() == 1 && arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Source.front().URI == arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(arcJob.DataStaging.File.front().KeepData);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.front().Name);
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.back().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.front().Source.size() == 1 && xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(job.DataStaging.File.front().Source.front().URI == xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(!xrslJob.DataStaging.File.front().KeepData);
  XRSL_ASSERT(xrslJob.DataStaging.File.back().KeepData);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.front().Name);
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.back().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.front().Source.size() == 1 && jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Source.front().URI == jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(!jdlJob.DataStaging.File.front().KeepData);
  JDL_ASSERT(jdlJob.DataStaging.File.back().KeepData);
}

/** 6-Upload-Download */
void JobDescriptionTest::TestDataStagingUploadDownload() {
  job.DataStaging.File.clear();

  Arc::FileType file;
  file.Name = "6-Upload-Download";
  Arc::DataSourceType source;
  source.URI = file.Name;
  source.Threads = -1;
  file.Source.push_back(source);
  file.KeepData = true;
  job.DataStaging.File.push_back(file);

  // Needed by the XRSLParser.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << "6-Upload-Download";
  f.close();
  
  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().Source.size() == 1 && arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Source.front().URI == arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(arcJob.DataStaging.File.front().KeepData);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.front().Name);
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.back().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.front().Source.size() == 1 && xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(job.DataStaging.File.front().Source.front().URI == xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(!xrslJob.DataStaging.File.front().KeepData);
  XRSL_ASSERT(xrslJob.DataStaging.File.back().KeepData);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.front().Name);
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.back().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.front().Source.size() == 1 && jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Source.front().URI == jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(!jdlJob.DataStaging.File.front().KeepData);
  JDL_ASSERT(jdlJob.DataStaging.File.back().KeepData);
}

/** 7-Create-Upload */
void JobDescriptionTest::TestDataStagingCreateUpload() {
  job.DataStaging.File.clear();

  Arc::FileType file;
  file.Name = "7-Create-Upload";
  Arc::DataTargetType target;
  target.URI = "http://example.com/" + file.Name;
  target.Threads = -1;
  file.Target.push_back(target);
  file.KeepData = false;
  job.DataStaging.File.push_back(file);

  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().Target.size() == 1 && arcJob.DataStaging.File.front().Target.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Target.front().URI == arcJob.DataStaging.File.front().Target.front().URI);
  ARC_ASSERT(!arcJob.DataStaging.File.front().KeepData);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.back().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.back().Target.size() == 1 && xrslJob.DataStaging.File.back().Target.front().URI);
  XRSL_ASSERT(job.DataStaging.File.front().Target.front().URI == xrslJob.DataStaging.File.back().Target.front().URI);
  XRSL_ASSERT(!xrslJob.DataStaging.File.back().KeepData);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.back().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.back().Target.size() == 1 && jdlJob.DataStaging.File.back().Target.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Target.front().URI == jdlJob.DataStaging.File.back().Target.front().URI);
  JDL_ASSERT(!jdlJob.DataStaging.File.back().KeepData);
}

/** 8-Download-Upload */
void JobDescriptionTest::TestDataStagingDownloadUpload() {
  job.DataStaging.File.clear();

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
  job.DataStaging.File.push_back(file);

  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().Source.size() == 1 && arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Source.front().URI == arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(arcJob.DataStaging.File.front().Target.size() == 1 && arcJob.DataStaging.File.front().Target.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Target.front().URI == arcJob.DataStaging.File.front().Target.front().URI);
  ARC_ASSERT(!arcJob.DataStaging.File.front().KeepData);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.front().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.front().Source.size() == 1 && xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(job.DataStaging.File.front().Source.front().URI == xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.back().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.back().Target.size() == 1 && xrslJob.DataStaging.File.back().Target.front().URI);
  XRSL_ASSERT(job.DataStaging.File.back().Target.front().URI == xrslJob.DataStaging.File.back().Target.front().URI);
  XRSL_ASSERT(!xrslJob.DataStaging.File.front().KeepData);
  XRSL_ASSERT(!xrslJob.DataStaging.File.back().KeepData);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.front().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.front().Source.size() == 1 && jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Source.front().URI == jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.back().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.back().Target.size() == 1 && jdlJob.DataStaging.File.back().Target.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Target.front().URI == jdlJob.DataStaging.File.back().Target.front().URI);
  JDL_ASSERT(!jdlJob.DataStaging.File.front().KeepData);
  JDL_ASSERT(!jdlJob.DataStaging.File.back().KeepData);
}

/** 9-Upload-Upload */
void JobDescriptionTest::TestDataStagingUploadUpload() {
  job.DataStaging.File.clear();

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
  job.DataStaging.File.push_back(file);

  // Needed by the XRSLParser.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << "9-Upload-Upload";
  f.close();
  
  arcJob.Parse(job.UnParse("ARCJSDL"));
  ARC_ASSERT(arcJob);
  ARC_ASSERT(arcJob.DataStaging.File.size() == 1);
  ARC_ASSERT_EQUAL(job.DataStaging.File.front().Name, arcJob.DataStaging.File.front().Name);
  ARC_ASSERT(arcJob.DataStaging.File.front().Source.size() == 1 && arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Source.front().URI == arcJob.DataStaging.File.front().Source.front().URI);
  ARC_ASSERT(arcJob.DataStaging.File.front().Target.size() == 1 && arcJob.DataStaging.File.front().Target.front().URI);
  ARC_ASSERT(job.DataStaging.File.front().Target.front().URI == arcJob.DataStaging.File.front().Target.front().URI);
  ARC_ASSERT(!arcJob.DataStaging.File.front().KeepData);

  xrslJob.Parse(job.UnParse("XRSL"));
  XRSL_ASSERT(xrslJob);
  XRSL_ASSERT(!xrslJob.DataStaging.File.empty());
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.front().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.front().Source.size() == 1 && xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT(job.DataStaging.File.front().Source.front().URI == xrslJob.DataStaging.File.front().Source.front().URI);
  XRSL_ASSERT_EQUAL(job.DataStaging.File.front().Name, xrslJob.DataStaging.File.back().Name);
  XRSL_ASSERT(xrslJob.DataStaging.File.back().Target.size() == 1 && xrslJob.DataStaging.File.back().Target.front().URI);
  XRSL_ASSERT(job.DataStaging.File.back().Target.front().URI == xrslJob.DataStaging.File.back().Target.front().URI);
  XRSL_ASSERT(!xrslJob.DataStaging.File.front().KeepData);
  XRSL_ASSERT(!xrslJob.DataStaging.File.back().KeepData);

  jdlJob.Parse(job.UnParse("JDL"));
  JDL_ASSERT(jdlJob);
  JDL_ASSERT(!jdlJob.DataStaging.File.empty());
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.front().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.front().Source.size() == 1 && jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Source.front().URI == jdlJob.DataStaging.File.front().Source.front().URI);
  JDL_ASSERT_EQUAL(job.DataStaging.File.front().Name, jdlJob.DataStaging.File.back().Name);
  JDL_ASSERT(jdlJob.DataStaging.File.back().Target.size() == 1 && jdlJob.DataStaging.File.back().Target.front().URI);
  JDL_ASSERT(job.DataStaging.File.front().Target.front().URI == jdlJob.DataStaging.File.back().Target.front().URI);
  JDL_ASSERT(!jdlJob.DataStaging.File.front().KeepData);
  JDL_ASSERT(!jdlJob.DataStaging.File.back().KeepData);
}

void JobDescriptionTest::TestPOSIXCompliance() {
  /** Testing compliance with GFD 56 - the POSIX extension **/

  const std::string posixJSDLStr = "<?xml version=\"1.0\"?>"
"<JobDefinition>"
"<JobDescription>"
"<Application>"
"<posix-jsdl:POSIXApplication>"
"<posix-jsdl:Executable>executable</posix-jsdl:Executable>"
"<posix-jsdl:Argument>arg1</posix-jsdl:Argument>"
"<posix-jsdl:Argument>arg2</posix-jsdl:Argument>"
"<posix-jsdl:Argument>arg3</posix-jsdl:Argument>"
"<posix-jsdl:Input>input</posix-jsdl:Input>"
"<posix-jsdl:Output>output</posix-jsdl:Output>"
"<posix-jsdl:Error>error</posix-jsdl:Error>"
"<posix-jsdl:Environment name=\"var1\">value1</posix-jsdl:Environment>"
"<posix-jsdl:Environment name=\"var2\">value2</posix-jsdl:Environment>"
"<posix-jsdl:Environment name=\"var3\">value3</posix-jsdl:Environment>"
"<posix-jsdl:WallTimeLimit>50</posix-jsdl:WallTimeLimit>"
"<posix-jsdl:MemoryLimit>100</posix-jsdl:MemoryLimit>"
"<posix-jsdl:CPUTimeLimit>110</posix-jsdl:CPUTimeLimit>"
"<posix-jsdl:ProcessCountLimit>2</posix-jsdl:ProcessCountLimit>"
"<posix-jsdl:VirtualMemoryLimit>500</posix-jsdl:VirtualMemoryLimit>"
"<posix-jsdl:ThreadCountLimit>7</posix-jsdl:ThreadCountLimit>"
"</posix-jsdl:POSIXApplication>"
"</Application>"
"</JobDescription>"
"</JobDefinition>";

  job.Application.Input = "input";
  job.Application.Output = "output";
  job.Application.Error = "error";

  job.Application.Environment.push_back(std::make_pair("var1", "value1"));
  job.Application.Environment.push_back(std::make_pair("var2", "value2"));
  job.Application.Environment.push_back(std::make_pair("var3", "value3"));

  job.Resources.TotalWallTime = 50;
  job.Resources.IndividualPhysicalMemory = 100;
  job.Resources.TotalCPUTime = 110;
  job.Resources.SlotRequirement.NumberOfSlots = 2;
  job.Resources.IndividualVirtualMemory = 500;
  job.Resources.SlotRequirement.ThreadsPerProcesses = 7;

  arcJob.Parse(posixJSDLStr);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Executable.Name, arcJob.Application.Executable.Name);
  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", job.Application.Executable.Argument == arcJob.Application.Executable.Argument);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Input, arcJob.Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Output, arcJob.Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Error, arcJob.Application.Error);
  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", job.Application.Environment == arcJob.Application.Environment);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.TotalWallTime.range.max, arcJob.Resources.TotalWallTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.IndividualPhysicalMemory.max, arcJob.Resources.IndividualPhysicalMemory.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.TotalCPUTime.range.max, arcJob.Resources.TotalCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.SlotRequirement.NumberOfSlots.max, arcJob.Resources.SlotRequirement.NumberOfSlots.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.IndividualVirtualMemory.max, arcJob.Resources.IndividualVirtualMemory.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.SlotRequirement.ThreadsPerProcesses.max, arcJob.Resources.SlotRequirement.ThreadsPerProcesses.max);

  Arc::XMLNode xmlArcJSDL(arcJob.UnParse("ARCJSDL"));
  Arc::XMLNode pApp = xmlArcJSDL["JobDescription"]["Application"]["POSIXApplication"];

  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", pApp);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Executable.Name, (std::string)pApp["Executable"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"arg1", (std::string)pApp["Argument"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"arg2", (std::string)pApp["Argument"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"arg3", (std::string)pApp["Argument"][2]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Input, (std::string)pApp["Input"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Output, (std::string)pApp["Output"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Application.Error, (std::string)pApp["Error"]);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"var1",   (std::string)pApp["Environment"][0].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"value1", (std::string)pApp["Environment"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"var2",   (std::string)pApp["Environment"][1].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"value2", (std::string)pApp["Environment"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"var3",   (std::string)pApp["Environment"][2].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"value3", (std::string)pApp["Environment"][2]);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.TotalWallTime.range.max, Arc::stringto<int>(pApp["WallTimeLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.IndividualPhysicalMemory.max, Arc::stringto<int64_t>(pApp["MemoryLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.TotalCPUTime.range.max, Arc::stringto<int>(pApp["CPUTimeLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.SlotRequirement.NumberOfSlots.max, Arc::stringto<int>(pApp["ProcessCountLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.IndividualVirtualMemory.max, Arc::stringto<int64_t>(pApp["VirtualMemoryLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", job.Resources.SlotRequirement.ThreadsPerProcesses.max, Arc::stringto<int>(pApp["ThreadCountLimit"]));
}

void JobDescriptionTest::TestHPCCompliance() {
  /** Testing compliance with GFD 111 **/

  const std::string hpcJSDLStr = "<?xml version=\"1.0\"?>"
"<JobDefinition>"
"<JobDescription>"
"<Application>"
"<HPC-jsdl:HPCProfileApplication>"
"<HPC-jsdl:Executable>executable</HPC-jsdl:Executable>"
"<HPC-jsdl:Argument>arg1</HPC-jsdl:Argument>"
"<HPC-jsdl:Argument>arg2</HPC-jsdl:Argument>"
"<HPC-jsdl:Argument>arg3</HPC-jsdl:Argument>"
"<HPC-jsdl:Input>input</HPC-jsdl:Input>"
"<HPC-jsdl:Output>output</HPC-jsdl:Output>"
"<HPC-jsdl:Error>error</HPC-jsdl:Error>"
"<HPC-jsdl:Environment name=\"var1\">value1</HPC-jsdl:Environment>"
"<HPC-jsdl:Environment name=\"var2\">value2</HPC-jsdl:Environment>"
"<HPC-jsdl:Environment name=\"var3\">value3</HPC-jsdl:Environment>"
"<HPC-jsdl:WallTimeLimit>50</HPC-jsdl:WallTimeLimit>"
"<HPC-jsdl:MemoryLimit>100</HPC-jsdl:MemoryLimit>"
"<HPC-jsdl:CPUTimeLimit>110</HPC-jsdl:CPUTimeLimit>"
"<HPC-jsdl:ProcessCountLimit>2</HPC-jsdl:ProcessCountLimit>"
"<HPC-jsdl:VirtualMemoryLimit>500</HPC-jsdl:VirtualMemoryLimit>"
"<HPC-jsdl:ThreadCountLimit>7</HPC-jsdl:ThreadCountLimit>"
"</HPC-jsdl:HPCProfileApplication>"
"</Application>"
"</JobDescription>"
"</JobDefinition>";

  job.Application.Input = "input";
  job.Application.Output = "output";
  job.Application.Error = "error";

  job.Application.Environment.push_back(std::make_pair("var1", "value1"));
  job.Application.Environment.push_back(std::make_pair("var2", "value2"));
  job.Application.Environment.push_back(std::make_pair("var3", "value3"));

  arcJob.Parse(hpcJSDLStr);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Executable.Name, arcJob.Application.Executable.Name);
  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", job.Application.Executable.Argument == arcJob.Application.Executable.Argument);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Input, arcJob.Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Output, arcJob.Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Error, arcJob.Application.Error);
  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", job.Application.Environment == arcJob.Application.Environment);

  Arc::XMLNode xmlArcJSDL(arcJob.UnParse("ARCJSDL"));
  Arc::XMLNode pApp = xmlArcJSDL["JobDescription"]["Application"]["HPCProfileApplication"];

  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", pApp);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Executable.Name, (std::string)pApp["Executable"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"arg1", (std::string)pApp["Argument"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"arg2", (std::string)pApp["Argument"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"arg3", (std::string)pApp["Argument"][2]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Input, (std::string)pApp["Input"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Output, (std::string)pApp["Output"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", job.Application.Error, (std::string)pApp["Error"]);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"var1",   (std::string)pApp["Environment"][0].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"value1", (std::string)pApp["Environment"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"var2",   (std::string)pApp["Environment"][1].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"value2", (std::string)pApp["Environment"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"var3",   (std::string)pApp["Environment"][2].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"value3", (std::string)pApp["Environment"][2]);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JobDescriptionTest);
