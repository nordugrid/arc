#include <string>
#include <fstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>
#include <arc/client/JobDescription.h>

#include "../XRSLParser.h"


class XRSLParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(XRSLParserTest);
  CPPUNIT_TEST(TestExecutable);
  CPPUNIT_TEST(TestInputOutputError);
  CPPUNIT_TEST(TestFilesDownloadDelete);
  CPPUNIT_TEST(TestFilesUploadDelete);
  CPPUNIT_TEST(TestFilesCreateDownload);
  CPPUNIT_TEST(TestFilesDownloadDownload);
  CPPUNIT_TEST(TestFilesUploadDownload);
  CPPUNIT_TEST(TestFilesCreateUpload);
  CPPUNIT_TEST(TestFilesDownloadUpload);
  CPPUNIT_TEST(TestFilesUploadUpload);
  CPPUNIT_TEST(TestExecutables);
  CPPUNIT_TEST(TestFTPThreads);
  CPPUNIT_TEST(TestCache);
  CPPUNIT_TEST(TestQueue);
  CPPUNIT_TEST(TestNotify);
  CPPUNIT_TEST(TestJoin);
  CPPUNIT_TEST(TestDryRun);
  CPPUNIT_TEST(TestGridTime);
  CPPUNIT_TEST(TestAdditionalAttributes);
  CPPUNIT_TEST(TestMultiRSL);
  CPPUNIT_TEST(TestDisjunctRSL);
  CPPUNIT_TEST_SUITE_END();

public:
  XRSLParserTest() {}

  void setUp();
  void tearDown();
  void TestExecutable();
  void TestInputOutputError();
  void TestFilesDownloadDelete();
  void TestFilesUploadDelete();
  void TestFilesCreateDownload();
  void TestFilesDownloadDownload();
  void TestFilesUploadDownload();
  void TestFilesCreateUpload();
  void TestFilesDownloadUpload();
  void TestFilesUploadUpload();
  void TestExecutables();
  void TestFTPThreads();
  void TestCache();
  void TestQueue();
  void TestNotify();
  void TestJoin();
  void TestDryRun();
  void TestGridTime();
  void TestAdditionalAttributes();
  void TestMultiRSL();
  void TestDisjunctRSL();

private:
  Arc::JobDescription INJOB;
  std::list<Arc::JobDescription> OUTJOBS;
  Arc::XRSLParser PARSER;

  std::string MESSAGE;
  std::string xrsl;
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
  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Executable.Name, OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Executable.Argument, OUTJOBS.front().Application.Executable.Argument);
}

void XRSLParserTest::TestInputOutputError() {
  INJOB.Application.Input = "input-file";
  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(INJOB.Application.Input.c_str(), std::ifstream::trunc);
  f << INJOB.Application.Input;
  f.close();

  INJOB.Application.Output = "output-file";
  INJOB.Application.Error = "error-file";

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Input, OUTJOBS.front().Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Output, OUTJOBS.front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Error, OUTJOBS.front().Application.Error);

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
 * In addition in the JobDescription the "IsExecutable" can be specified for a
 * file, these are not fully tested yet.
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
void XRSLParserTest::TestFilesDownloadDelete() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing download-delete data staging type.";

  Arc::FileType file;
  file.Name = "2-Download-Delete";
  file.Source.push_back(Arc::URL("http://example.com/" + file.Name));
  file.KeepData = false;
  INJOB.Files.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Source.back(),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  */
}

/** 3-Upload-Delete */
void XRSLParserTest::TestFilesUploadDelete() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing upload-delete data staging type.";

  Arc::FileType file;
  file.Name = "3-Upload-Delete";
  file.Source.push_back(Arc::URL(file.Name));
  file.KeepData = false;
  INJOB.Files.push_back(file);

  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << file.Name;
  f.close();

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Source.back(),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  */

  remove(file.Name.c_str());
}

/** 4-Create-Download */
void XRSLParserTest::TestFilesCreateDownload() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing create-download data staging type.";

  Arc::FileType file;
  file.Name = "4-Create-Download";
  file.KeepData = true;
  INJOB.Files.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();
  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front().URI);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  it++;
  */

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
}

/** 5-Download-Download */
void XRSLParserTest::TestFilesDownloadDownload() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing download-download data staging type.";

  Arc::FileType file;
  file.Name = "5-Download-Download";
  file.Source.push_back(Arc::URL("http://example.com/" + file.Name));
  file.KeepData = true;
  INJOB.Files.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Source.back(),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0, (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  */

  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
}

/** 6-Upload-Download */
void XRSLParserTest::TestFilesUploadDownload() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing upload-download data staging type.";

  Arc::FileType file;
  file.Name = "6-Upload-Download";
  file.Source.push_back(Arc::URL(file.Name));
  file.KeepData = true;
  INJOB.Files.push_back(file);

  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << file.Name;
  f.close();

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Source.back(),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  */

  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());

  remove(file.Name.c_str());
}

/** 7-Create-Upload */
void XRSLParserTest::TestFilesCreateUpload() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing create-download data staging type.";

  Arc::FileType file;
  file.Name = "7-Create-Upload";
  file.Target.push_back(Arc::URL("http://example.com/" + file.Name));
  file.KeepData = false;
  INJOB.Files.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  it++;
  */

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Target.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Target.back(),  it->Target.front());
}

/** 8-Download-Upload */
void XRSLParserTest::TestFilesDownloadUpload() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing download-upload data staging type.";

  Arc::FileType file;
  file.Name = "8-Download-Upload";
  file.Source.push_back(Arc::URL("http://example.com/" + file.Name));
  file.Target.push_back(Arc::URL("http://example.com/" + file.Name));
  file.KeepData = false;
  INJOB.Files.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Source.back(),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  */

  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Target.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Target.back(),  it->Target.front());
}

/** 9-Upload-Upload */
void XRSLParserTest::TestFilesUploadUpload() {
  INJOB.Files.clear();
  MESSAGE = "Error parsing upload-upload data staging type.";

  Arc::FileType file;
  file.Name = "9-Upload-Upload";
  file.Source.push_back(Arc::URL(file.Name));
  file.Target.push_back(Arc::URL("http://example.com/" + file.Name));
  file.KeepData = false;
  INJOB.Files.push_back(file);

  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << file.Name;
  f.close();

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)OUTJOBS.front().Files.size());

  std::list<Arc::FileType>::const_iterator it = OUTJOBS.front().Files.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Source.front(),  it->Source.back());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());

  /* When unparsing the job description, the executable will not be added. The
   * Submitter::ModifyJobDescription(ExecutionTarget) method need to be called
   * to add the executable. This might change in the future.
  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable",  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, false,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::URL("executable"),  it->Source.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Target.size());
  */

  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.KeepData,  it->KeepData);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Source.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Target.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Target.front(),  it->Target.front());

  remove(file.Name.c_str());
}

void XRSLParserTest::TestExecutables() {
  xrsl = "&(executable=/bin/true)(|(executables=\"in1\")(executables=\"in2\"))(inputfiles=(\"in1\" \"\") (\"in2\" \"\"))";

  std::ofstream f("in1", std::ifstream::trunc);
  f << "in1";
  f.close();
  f.open("in2", std::ifstream::trunc);
  f << "in2";
  f.close();

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().Files.front().Name);
  CPPUNIT_ASSERT(OUTJOBS.front().Files.front().IsExecutable);
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().Files.back().Name);
  CPPUNIT_ASSERT(!OUTJOBS.front().Files.back().IsExecutable);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().GetAlternatives().front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().GetAlternatives().front().Files.front().Name);
  CPPUNIT_ASSERT(!OUTJOBS.front().GetAlternatives().front().Files.front().IsExecutable);
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().GetAlternatives().front().Files.back().Name);
  CPPUNIT_ASSERT(OUTJOBS.front().GetAlternatives().front().Files.back().IsExecutable);

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().Files.front().Name);
  CPPUNIT_ASSERT(OUTJOBS.front().Files.front().IsExecutable);
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().Files.back().Name);
  CPPUNIT_ASSERT(!OUTJOBS.front().Files.back().IsExecutable);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  xrsl = "&(executable=/bin/true)(|(executables=\"non-existing\"))(inputfiles=(\"in1\" \"\"))";
  CPPUNIT_ASSERT(!PARSER.Parse(xrsl, OUTJOBS));

  remove("in1");
  remove("in2");
}

void XRSLParserTest::TestCache() {
  xrsl = "&(executable=\"executable\")"
          "(inputfiles=(\"in1\" \"gsiftp://example.com/in1\") (\"in2\" \"gsiftp://example.com/in2\"))"
          "(|(cache=yes)(cache=copy))";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().Files.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.front().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().Files.front().Source.front().Option("cache"));
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().Files.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.back().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().Files.back().Source.front().Option("cache"));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().GetAlternatives().front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().GetAlternatives().front().Files.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().Files.front().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"copy", OUTJOBS.front().GetAlternatives().front().Files.front().Source.front().Option("cache"));
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().GetAlternatives().front().Files.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().Files.back().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"copy", OUTJOBS.front().GetAlternatives().front().Files.back().Source.front().Option("cache"));

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().Files.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.front().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().Files.front().Source.front().Option("cache"));
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().Files.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.back().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().Files.back().Source.front().Option("cache"));

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());
}

void XRSLParserTest::TestQueue() {
  xrsl = "&(executable=\"executable\")"
          "(|(queue=q1)(queue=q2))";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"q1", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"q2", OUTJOBS.front().GetAlternatives().front().Resources.QueueName);

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"q1", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  xrsl = "&(executable=\"executable\")"
          "(|(queue!=q1)(queue!=q2))";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  std::map<std::string, std::string>::const_iterator itAttribute;
  itAttribute = OUTJOBS.front().OtherAttributes.find("nordugrid:broker;reject_queue");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() != itAttribute);
  CPPUNIT_ASSERT_EQUAL((std::string)"q1", itAttribute->second);

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  itAttribute = OUTJOBS.front().GetAlternatives().front().OtherAttributes.find("nordugrid:broker;reject_queue");
  CPPUNIT_ASSERT(OUTJOBS.front().GetAlternatives().front().OtherAttributes.end() != itAttribute);
  CPPUNIT_ASSERT_EQUAL((std::string)"q2", itAttribute->second);

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  itAttribute = OUTJOBS.front().OtherAttributes.find("nordugrid:broker;reject_queue");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() == itAttribute);

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());
}

void XRSLParserTest::TestFTPThreads() {
  xrsl = "&(executable=\"executable\")"
          "(inputfiles=(\"in\" \"gsiftp://example.com/in\"))"
          "(outputfiles=(\"out\" \"gsiftp://example.com/out\"))"
          "(|(ftpthreads=5)(ftpthreads=3))";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in", OUTJOBS.front().Files.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.front().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().Files.front().Source.front().Option("threads"));
  CPPUNIT_ASSERT_EQUAL((std::string)"out", OUTJOBS.front().Files.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.back().Target.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().Files.back().Target.front().Option("threads"));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().GetAlternatives().front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in", OUTJOBS.front().GetAlternatives().front().Files.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().Files.front().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"3", OUTJOBS.front().GetAlternatives().front().Files.front().Source.front().Option("threads"));
  CPPUNIT_ASSERT_EQUAL((std::string)"out", OUTJOBS.front().GetAlternatives().front().Files.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().Files.back().Target.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"3", OUTJOBS.front().GetAlternatives().front().Files.back().Target.front().Option("threads"));

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Files.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in", OUTJOBS.front().Files.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.front().Source.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().Files.front().Source.front().Option("threads"));
  CPPUNIT_ASSERT_EQUAL((std::string)"out", OUTJOBS.front().Files.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Files.back().Target.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().Files.back().Target.front().Option("threads"));

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  xrsl = "&(executable=\"executable\")"
          "(inputfiles=(\"in\" \"gsiftp://example.com/in\"))"
          "(outputfiles=(\"out\" \"gsiftp://example.com/out\"))"
          "(ftpthreads=20)";
  CPPUNIT_ASSERT(!PARSER.Parse(xrsl, OUTJOBS));
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
  xrsl = "&(executable = \"executable\")(notify = \"someone@example.com\")";

  std::list<Arc::JobDescription> tempJobDescs;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT_EQUAL(1, (int)tempJobDescs.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(tempJobDescs.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)tempJobDescs.front().Application.Notification.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().Application.Notification.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"someone@example.com",  tempJobDescs.front().Application.Notification.front().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"someone@example.com",  OUTJOBS.front().Application.Notification.front().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)tempJobDescs.front().Application.Notification.front().States.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)OUTJOBS.front().Application.Notification.front().States.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  tempJobDescs.front().Application.Notification.front().States.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHED",  tempJobDescs.front().Application.Notification.front().States.back());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  OUTJOBS.front().Application.Notification.front().States.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHED",  OUTJOBS.front().Application.Notification.front().States.back());

  // Test all flags.
  xrsl = "&(executable = \"executable\")(notify = \"bqfedc someone@example.com\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT_EQUAL(1, (int)tempJobDescs.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(tempJobDescs.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)tempJobDescs.front().Application.Notification.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().Application.Notification.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"someone@example.com",  tempJobDescs.front().Application.Notification.front().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"someone@example.com",  OUTJOBS.front().Application.Notification.front().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 6,  (int)tempJobDescs.front().Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = tempJobDescs.front().Application.Notification.front().States.begin();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"INLRMS",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"DELETED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"CANCELING",  *it);
  }
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 6,  (int)OUTJOBS.front().Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = OUTJOBS.front().Application.Notification.front().States.begin();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"INLRMS",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"DELETED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"CANCELING",  *it);
  }

  // Test multiple entries and overlapping states.
  xrsl = "&(executable = \"executable\")(notify = \"bqfedc someone@example.com\" \"bqf someone@example.com anotherone@example.com\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT_EQUAL(1, (int)tempJobDescs.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(tempJobDescs.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)tempJobDescs.front().Application.Notification.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2,  (int)OUTJOBS.front().Application.Notification.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"someone@example.com",  tempJobDescs.front().Application.Notification.front().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"someone@example.com",  OUTJOBS.front().Application.Notification.front().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"anotherone@example.com",  tempJobDescs.front().Application.Notification.back().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"anotherone@example.com",  OUTJOBS.front().Application.Notification.back().Email);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 6,  (int)tempJobDescs.front().Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = tempJobDescs.front().Application.Notification.front().States.begin();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"INLRMS",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"DELETED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"CANCELING",  *it);
  }
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 6,  (int)OUTJOBS.front().Application.Notification.front().States.size());
  {
    std::list<std::string>::const_iterator it = OUTJOBS.front().Application.Notification.front().States.begin();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"INLRMS",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"DELETED",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"CANCELING",  *it);
  }
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 3,  (int)tempJobDescs.front().Application.Notification.back().States.size());
  {
    std::list<std::string>::const_iterator it = tempJobDescs.front().Application.Notification.front().States.begin();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"INLRMS",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHING",  *it);
  }
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 3,  (int)OUTJOBS.front().Application.Notification.back().States.size());
  {
    std::list<std::string>::const_iterator it = OUTJOBS.front().Application.Notification.front().States.begin();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"PREPARING",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"INLRMS",  *it++);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"FINISHING",  *it);
  }

  // Test invalid email address.
  xrsl = "&(executable = \"executable\")(notify = \"someoneAexample.com\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT(tempJobDescs.empty());

  // Test invalid email address with state flags.
  xrsl = "&(executable = \"executable\")(notify = \"bqfecd someoneAexample.com\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT(tempJobDescs.empty());

  // Test unknown state flags.
  xrsl = "&(executable = \"executable\")(notify = \"xyz someone@example.com\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT(tempJobDescs.empty());
}

void XRSLParserTest::TestDryRun() {
  MESSAGE = "Error parsing the dryrun attribute.";

  xrsl = "&(|(executable = \"executable\")(executable = \"executable\"))(dryrun = \"yes\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().Application.DryRun);

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().GetAlternatives().front().Application.DryRun);

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().Application.DryRun);

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  xrsl = "&(|(executable = \"executable\")(executable = \"executable\"))(dryrun = \"no\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !OUTJOBS.front().Application.DryRun);

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !OUTJOBS.front().GetAlternatives().front().Application.DryRun);

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !OUTJOBS.front().Application.DryRun);

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());
}

void XRSLParserTest::TestJoin() {
  MESSAGE = "Error parsing the join attribute.";

  xrsl = "&(executable = \"executable\")(join = \"yes\")(|(stdout = \"output-file\")(stdout = \"output-file2\"))";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"output-file",  OUTJOBS.front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"output-file",  OUTJOBS.front().Application.Error);

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"output-file2",  OUTJOBS.front().GetAlternatives().front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"output-file2",  OUTJOBS.front().GetAlternatives().front().Application.Error);

  xrsl = "&(executable = \"executable\")(stderr = \"error-file\")(join = \"yes\")";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, OUTJOBS));

  xrsl = "&(executable = \"executable\")(stdout = \"output-file\")(stderr = \"error-file\")(join = \"yes\")";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, OUTJOBS));

  xrsl = "&(executable = \"executable\")(stdout = \"output-file\")(stderr = \"error-file\")(join = \"no\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"output-file", OUTJOBS.front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"error-file",  OUTJOBS.front().Application.Error);
}

void XRSLParserTest::TestGridTime() {
  xrsl = "&(executable=/bin/echo)(gridtime=600s)";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.TotalCPUTime.range.min);
  CPPUNIT_ASSERT_EQUAL(600, OUTJOBS.front().Resources.TotalCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL((std::string)"clock rate", OUTJOBS.front().Resources.TotalCPUTime.benchmark.first);
  CPPUNIT_ASSERT_EQUAL(2800., OUTJOBS.front().Resources.TotalCPUTime.benchmark.second);
}

void XRSLParserTest::TestAdditionalAttributes() {
  std::string tmpjobdesc;

  INJOB.OtherAttributes["nordugrid:xrsl;hostname"] = "localhost";
  INJOB.OtherAttributes["nordugrid:xrsl;unknownattribute"] = "none";
  INJOB.OtherAttributes["bogus:nonexisting;foo"] = "bar";
  CPPUNIT_ASSERT(PARSER.UnParse(INJOB, tmpjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT(PARSER.Parse(tmpjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  std::map<std::string, std::string>::const_iterator itAttribute;
  itAttribute = OUTJOBS.front().OtherAttributes.find("nordugrid:xrsl;hostname");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() != itAttribute);
  CPPUNIT_ASSERT_EQUAL((std::string)"localhost", itAttribute->second);
  itAttribute = OUTJOBS.front().OtherAttributes.find("nordugrid:xrsl;unknownattribute");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() == itAttribute);
  itAttribute = OUTJOBS.front().OtherAttributes.find("bogus:nonexisting;foo");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() == itAttribute);
}

void XRSLParserTest::TestMultiRSL() {
  xrsl = "+(&(executable= \"/bin/exe1\"))(&(executable= \"/bin/exe2\"))";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe1", OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe2", OUTJOBS.back().Application.Executable.Name);
}

void XRSLParserTest::TestDisjunctRSL() {
  xrsl = "&(executable=\"/bin/exe\")"
          "(|(|(queue=\"q1.1\")"
              "(|(queue=\"q1.2.1\")"
                "(queue=\"q1.2.2\")"
              ")"
            ")"
            "(queue=\"q2\")"
            "(queue=\"q3\")"
          ")"
          "(arguments=\"Hello world!\")";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.1", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.2.1", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.2.2", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q2", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q3", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(!OUTJOBS.front().UseAlternative());
  OUTJOBS.front().UseOriginal();
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.1", OUTJOBS.front().Resources.QueueName);
}

CPPUNIT_TEST_SUITE_REGISTRATION(XRSLParserTest);
