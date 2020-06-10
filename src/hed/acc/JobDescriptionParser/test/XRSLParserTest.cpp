#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>

#include <list>
std::ostream& operator<<(std::ostream& os, const std::list<std::string>& strings);

#include <cppunit/extensions/HelperMacros.h>

#include <arc/ArcLocation.h>
#include <arc/StringConv.h>
#include <arc/compute/JobDescription.h>

#include "../XRSLParser.h"


class XRSLParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(XRSLParserTest);
  CPPUNIT_TEST(TestExecutable);
  CPPUNIT_TEST(TestInputOutputError);
  CPPUNIT_TEST(TestInputFileClientStageable);
  CPPUNIT_TEST(TestInputFileServiceStageable);
  CPPUNIT_TEST(TestOutputFileClientStageable);
  CPPUNIT_TEST(TestOutputFileServiceStageable);
  CPPUNIT_TEST(TestURIOptionsInput);
  CPPUNIT_TEST(TestURIOptionsOutput);
  CPPUNIT_TEST(TestExecutables);
  CPPUNIT_TEST(TestFTPThreads);
  CPPUNIT_TEST(TestCache);
  CPPUNIT_TEST(TestQueue);
  CPPUNIT_TEST(TestNotify);
  CPPUNIT_TEST(TestJoin);
  CPPUNIT_TEST(TestDryRun);
  CPPUNIT_TEST(TestGridTime);
  CPPUNIT_TEST(TestAccessControl);
  CPPUNIT_TEST(TestParallelAttributes);
  CPPUNIT_TEST(TestAdditionalAttributes);
  CPPUNIT_TEST(TestMultiRSL);
  CPPUNIT_TEST(TestDisjunctRSL);
  CPPUNIT_TEST(TestRTE);
  CPPUNIT_TEST_SUITE_END();

public:
  XRSLParserTest():PARSER((Arc::PluginArgument*)NULL) {}

  void setUp();
  void tearDown();
  void TestExecutable();
  void TestInputOutputError();
  void TestInputFileClientStageable();
  void TestInputFileServiceStageable();
  void TestOutputFileClientStageable();
  void TestOutputFileServiceStageable();
  void TestURIOptionsInput();
  void TestURIOptionsOutput();
  void TestExecutables();
  void TestFTPThreads();
  void TestCache();
  void TestQueue();
  void TestNotify();
  void TestJoin();
  void TestDryRun();
  void TestGridTime();
  void TestAccessControl();
  void TestParallelAttributes();
  void TestAdditionalAttributes();
  void TestMultiRSL();
  void TestDisjunctRSL();
  void TestRTE();

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
  Arc::ArcLocation::Init("./bin/app");
  INJOB.Application.Executable.Path = "executable";
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
  // try empty executable
  xrsl = "&(name=test)";
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  OUTJOBS.clear();

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Executable.Path, OUTJOBS.front().Application.Executable.Path);
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

/** Client stageable input file */
void XRSLParserTest::TestInputFileClientStageable() {
  INJOB.DataStaging.InputFiles.clear();
  MESSAGE = "Error parsing TestInputFileClientStageable data staging type.";

  Arc::InputFileType file;
  file.Name = "TestInputFileClientStageable";
  file.Sources.push_back(Arc::URL(file.Name));
  file.FileSize = file.Name.length();
  INJOB.DataStaging.InputFiles.push_back(file);

  // The file need to be there, otherwise the XRSLParser will fail.
  std::ofstream f(file.Name.c_str(), std::ifstream::trunc);
  f << file.Name;
  f.close();

  std::string tempjobdesc;


  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.InputFiles.size());
  std::list<Arc::InputFileType>::const_iterator it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Sources.back(),  it->Sources.front());


  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl", "GRIDMANAGER"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS, "", "GRIDMANAGER"));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.InputFiles.size());
  it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.FileSize,  it->FileSize);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0, (int)it->Sources.size());


  // Remove source path
  INJOB.DataStaging.InputFiles.front().Sources.clear();


  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS, "", ""));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.InputFiles.size());
  it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Sources.size());


  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl", "GRIDMANAGER"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS, "", "GRIDMANAGER"));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.InputFiles.size());
  it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.FileSize,  it->FileSize);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0, (int)it->Sources.size());


  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl", "GRIDMANAGER"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS, "", "GRIDMANAGER"));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.InputFiles.size());
  it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0, (int)it->Sources.size());


  remove(file.Name.c_str());
}

/** Service stageable input file */
void XRSLParserTest::TestInputFileServiceStageable() {
  INJOB.DataStaging.InputFiles.clear();
  MESSAGE = "Error parsing TestInputFileServiceStageable data staging type.";

  Arc::InputFileType file;
  file.Name = "TestInputFileServiceStageable";
  file.Sources.push_back(Arc::URL("http://example.com/" + file.Name));
  INJOB.DataStaging.InputFiles.push_back(file);

  std::string tempjobdesc;

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.InputFiles.size());
  std::list<Arc::InputFileType>::const_iterator it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Sources.back(),  it->Sources.front());

  OUTJOBS.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl", "GRIDMANAGER"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS, "", "GRIDMANAGER"));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.InputFiles.size());
  it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Sources.back(),  it->Sources.front());
}

/** Client stageable output file */
void XRSLParserTest::TestOutputFileClientStageable() {
  INJOB.DataStaging.OutputFiles.clear();
  MESSAGE = "Error parsing TestOutputFileClientStageable data staging type.";

  Arc::OutputFileType file;
  file.Name = "TestOutputFileClientStageable";
  INJOB.DataStaging.OutputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.OutputFiles.size());

  std::list<Arc::OutputFileType>::const_iterator it = OUTJOBS.front().DataStaging.OutputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0,  (int)it->Targets.size());
}

/** Service stageable output file */
void XRSLParserTest::TestOutputFileServiceStageable() {
  INJOB.DataStaging.OutputFiles.clear();
  MESSAGE = "Error parsing create-download data staging type.";

  Arc::OutputFileType file;
  file.Name = "7-Create-Upload";
  file.Targets.push_back(Arc::URL("http://example.com/" + file.Name));
  INJOB.DataStaging.OutputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)OUTJOBS.front().DataStaging.OutputFiles.size());

  std::list<Arc::OutputFileType>::const_iterator it = OUTJOBS.front().DataStaging.OutputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,  it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1,  (int)it->Targets.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Targets.back(),  it->Targets.front());
}

void XRSLParserTest::TestURIOptionsInput() {
  xrsl = "&(executable=/bin/true)"
         "(inputfiles=(\"in1\" \"gsiftp://example.com/in1\" \"threads=5\"))";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.front().Sources.size());
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://example.com:2811/in1"), OUTJOBS.front().DataStaging.InputFiles.front().Sources.front().str());
  CPPUNIT_ASSERT_EQUAL(std::string("5"), OUTJOBS.front().DataStaging.InputFiles.front().Sources.front().Option("threads"));

  xrsl = "&(executable=/bin/true)"
         "(inputfiles=(\"in1\" \"lfc://example.com/in1\" \"location=gsiftp://example.com/in1\" \"threads=5\"))";

  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.front().Sources.size());

  const std::list<Arc::URLLocation> locations = OUTJOBS.front().DataStaging.InputFiles.front().Sources.front().Locations();
  CPPUNIT_ASSERT_EQUAL(1, (int)locations.size());
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://example.com:2811/in1"), locations.front().str());
  CPPUNIT_ASSERT_EQUAL(std::string("5"), locations.front().Option("threads"));

  xrsl = "&(executable=/bin/true)"
         "(inputfiles=(\"in1\" \"gsiftp://example.com/in1\" \"threads\"))";

  CPPUNIT_ASSERT(!PARSER.Parse(xrsl, OUTJOBS));
}

void XRSLParserTest::TestURIOptionsOutput() {
  xrsl = "&(executable=/bin/true)"
         "(outputfiles=(\"out1\" \"lfc://example.com/in1\" \"checksum=md5\" \"location=gsiftp://example.com/in1\" \"threads=5\" \"location=gsiftp://example2.com/in1\" \"threads=10\"))";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.OutputFiles.size());
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.OutputFiles.front().Targets.size());
  CPPUNIT_ASSERT_EQUAL(std::string("md5"), OUTJOBS.front().DataStaging.OutputFiles.front().Targets.front().Option("checksum"));

  const std::list<Arc::URLLocation> locations = OUTJOBS.front().DataStaging.OutputFiles.front().Targets.front().Locations();
  CPPUNIT_ASSERT_EQUAL(2, (int)locations.size());
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://example.com:2811/in1"), locations.front().str());
  CPPUNIT_ASSERT_EQUAL(std::string("5"), locations.front().Option("threads"));
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://example2.com:2811/in1"), locations.back().str());
  CPPUNIT_ASSERT_EQUAL(std::string("10"), locations.back().Option("threads"));
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

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT(OUTJOBS.front().DataStaging.InputFiles.front().IsExecutable);
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().DataStaging.InputFiles.back().Name);
  CPPUNIT_ASSERT(!OUTJOBS.front().DataStaging.InputFiles.back().IsExecutable);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT(!OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().IsExecutable);
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.back().Name);
  CPPUNIT_ASSERT(OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.back().IsExecutable);

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT(OUTJOBS.front().DataStaging.InputFiles.front().IsExecutable);
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().DataStaging.InputFiles.back().Name);
  CPPUNIT_ASSERT(!OUTJOBS.front().DataStaging.InputFiles.back().IsExecutable);
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

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.front().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().DataStaging.InputFiles.front().Sources.front().Option("cache"));
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().DataStaging.InputFiles.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.back().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().DataStaging.InputFiles.back().Sources.front().Option("cache"));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"copy", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().Sources.front().Option("cache"));
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.back().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"copy", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.back().Sources.front().Option("cache"));

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in1", OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.front().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().DataStaging.InputFiles.front().Sources.front().Option("cache"));
  CPPUNIT_ASSERT_EQUAL((std::string)"in2", OUTJOBS.front().DataStaging.InputFiles.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.back().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"yes", OUTJOBS.front().DataStaging.InputFiles.back().Sources.front().Option("cache"));

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
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"q1", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  xrsl = "&(executable=\"executable\")"
          "(|(queue!=q1)(queue!=q2))";

  OUTJOBS.clear();
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
  OUTJOBS.clear();
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

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in", OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.front().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().DataStaging.InputFiles.front().Sources.front().Option("threads"));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.OutputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"out", OUTJOBS.front().DataStaging.OutputFiles.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.OutputFiles.back().Targets.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().DataStaging.OutputFiles.back().Targets.front().Option("threads"));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"3", OUTJOBS.front().GetAlternatives().front().DataStaging.InputFiles.front().Sources.front().Option("threads"));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.OutputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"out", OUTJOBS.front().GetAlternatives().front().DataStaging.OutputFiles.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().front().DataStaging.OutputFiles.back().Targets.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"3", OUTJOBS.front().GetAlternatives().front().DataStaging.OutputFiles.back().Targets.front().Option("threads"));

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"in", OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.front().Sources.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().DataStaging.InputFiles.front().Sources.front().Option("threads"));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.OutputFiles.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"out", OUTJOBS.front().DataStaging.OutputFiles.back().Name);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.OutputFiles.back().Targets.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"5", OUTJOBS.front().DataStaging.OutputFiles.back().Targets.front().Option("threads"));

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

  tempJobDescs.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT_EQUAL(1, (int)tempJobDescs.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(tempJobDescs.front(), xrsl, "nordugrid:xrsl"));
  OUTJOBS.clear();
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

  tempJobDescs.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT_EQUAL(1, (int)tempJobDescs.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(tempJobDescs.front(), xrsl, "nordugrid:xrsl"));
  OUTJOBS.clear();
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

  tempJobDescs.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT(tempJobDescs.empty());

  // Test invalid email address with state flags.
  xrsl = "&(executable = \"executable\")(notify = \"bqfecd someoneAexample.com\")";

  tempJobDescs.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, tempJobDescs));
  CPPUNIT_ASSERT(tempJobDescs.empty());

  // Test unknown state flags.
  xrsl = "&(executable = \"executable\")(notify = \"xyz someone@example.com\")";

  tempJobDescs.clear();
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
  OUTJOBS.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().Application.DryRun);

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  xrsl = "&(|(executable = \"executable\")(executable = \"executable\"))(dryrun = \"no\")";

  OUTJOBS.clear();
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !OUTJOBS.front().Application.DryRun);

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !OUTJOBS.front().GetAlternatives().front().Application.DryRun);

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), xrsl, "nordugrid:xrsl"));
  OUTJOBS.clear();
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

  OUTJOBS.clear();
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

  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.TotalWallTime.range.min);
  CPPUNIT_ASSERT_EQUAL(600, OUTJOBS.front().Resources.TotalWallTime.range.max);
  CPPUNIT_ASSERT_EQUAL((std::string)"clock rate", OUTJOBS.front().Resources.TotalWallTime.benchmark.first);
  CPPUNIT_ASSERT_EQUAL(2800., OUTJOBS.front().Resources.TotalWallTime.benchmark.second);

  OUTJOBS.clear();


  xrsl = "&(executable=/bin/echo)(gridtime=600s)(count=1)";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.TotalCPUTime.range.min);
  CPPUNIT_ASSERT_EQUAL(600, OUTJOBS.front().Resources.TotalCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL((std::string)"clock rate", OUTJOBS.front().Resources.TotalCPUTime.benchmark.first);
  CPPUNIT_ASSERT_EQUAL(2800., OUTJOBS.front().Resources.TotalCPUTime.benchmark.second);

  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.TotalWallTime.range.min);
  CPPUNIT_ASSERT_EQUAL(600, OUTJOBS.front().Resources.TotalWallTime.range.max);
  CPPUNIT_ASSERT_EQUAL((std::string)"clock rate", OUTJOBS.front().Resources.TotalWallTime.benchmark.first);
  CPPUNIT_ASSERT_EQUAL(2800., OUTJOBS.front().Resources.TotalWallTime.benchmark.second);

  OUTJOBS.clear();


  xrsl = "&(executable=/bin/echo)(gridtime=600s)(count=5)";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.TotalCPUTime.range.min);
  CPPUNIT_ASSERT_EQUAL(600, OUTJOBS.front().Resources.TotalCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL((std::string)"clock rate", OUTJOBS.front().Resources.TotalCPUTime.benchmark.first);
  CPPUNIT_ASSERT_EQUAL(2800., OUTJOBS.front().Resources.TotalCPUTime.benchmark.second);

  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.TotalWallTime.range.min);
  CPPUNIT_ASSERT_EQUAL(3000, OUTJOBS.front().Resources.TotalWallTime.range.max);
  CPPUNIT_ASSERT_EQUAL((std::string)"clock rate", OUTJOBS.front().Resources.TotalWallTime.benchmark.first);
  CPPUNIT_ASSERT_EQUAL(2800., OUTJOBS.front().Resources.TotalWallTime.benchmark.second);

  OUTJOBS.clear();


  xrsl = "&(executable=/bin/echo)(gridtime=600s)(cputime=5s)";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, OUTJOBS));


  xrsl = "&(executable=/bin/echo)(gridtime=600s)(walltime=42s)";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, OUTJOBS));
}

void XRSLParserTest::TestAccessControl() {
  xrsl = "&(executable=/bin/echo)"
          "(acl='<?xml version=\"1.0\"?>"
                 "<gacl version=\"0.0.1\">"
                   "<entry>"
                     "<any-user></any-user>"
                     "<allow>"
                       "<write/>"
                       "<read/>"
                       "<list/>"
                       "<admin/>"
                     "</allow>"
                   "</entry>"
                "</gacl>')";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(OUTJOBS.front().Application.AccessControl);
  CPPUNIT_ASSERT_EQUAL((std::string)"gacl", OUTJOBS.front().Application.AccessControl.Name());
  CPPUNIT_ASSERT_EQUAL(1, OUTJOBS.front().Application.AccessControl.Size());
  CPPUNIT_ASSERT_EQUAL((std::string)"entry", OUTJOBS.front().Application.AccessControl.Child().Name());

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:xrsl"));
  OUTJOBS.clear();

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(OUTJOBS.front().Application.AccessControl);
  CPPUNIT_ASSERT_EQUAL((std::string)"gacl", OUTJOBS.front().Application.AccessControl.Name());
  CPPUNIT_ASSERT_EQUAL(1, OUTJOBS.front().Application.AccessControl.Size());
  CPPUNIT_ASSERT_EQUAL((std::string)"entry", OUTJOBS.front().Application.AccessControl.Child().Name());
}

void XRSLParserTest::TestParallelAttributes() {
  xrsl = "&(executable = \"/bin/echo\")"
          "(count = 8)"
          "(countpernode = 4)"
          "(exclusiveexecution = \"yes\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(8, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL(4, OUTJOBS.front().Resources.SlotRequirement.SlotsPerHost);
  CPPUNIT_ASSERT_EQUAL(Arc::SlotRequirementType::EE_TRUE, OUTJOBS.front().Resources.SlotRequirement.ExclusiveExecution);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:xrsl"));
  OUTJOBS.clear();

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(8, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL(4, OUTJOBS.front().Resources.SlotRequirement.SlotsPerHost);
  CPPUNIT_ASSERT_EQUAL(Arc::SlotRequirementType::EE_TRUE, OUTJOBS.front().Resources.SlotRequirement.ExclusiveExecution);
  OUTJOBS.clear();


  xrsl = "&(executable = \"/bin/echo\")"
          "(count = 8)"
          "(exclusiveexecution = \"no\")";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(8, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.SlotRequirement.SlotsPerHost);
  CPPUNIT_ASSERT_EQUAL(Arc::SlotRequirementType::EE_FALSE, OUTJOBS.front().Resources.SlotRequirement.ExclusiveExecution);

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:xrsl"));
  OUTJOBS.clear();

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(8, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.SlotRequirement.SlotsPerHost);
  CPPUNIT_ASSERT_EQUAL(Arc::SlotRequirementType::EE_FALSE, OUTJOBS.front().Resources.SlotRequirement.ExclusiveExecution);
  OUTJOBS.clear();


  xrsl = "&(executable = \"/bin/echo\")"
          "(count = 8)";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(8, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.SlotRequirement.SlotsPerHost);
  CPPUNIT_ASSERT_EQUAL(Arc::SlotRequirementType::EE_DEFAULT, OUTJOBS.front().Resources.SlotRequirement.ExclusiveExecution);

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:xrsl"));
  OUTJOBS.clear();

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(8, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL(-1, OUTJOBS.front().Resources.SlotRequirement.SlotsPerHost);
  CPPUNIT_ASSERT_EQUAL(Arc::SlotRequirementType::EE_DEFAULT, OUTJOBS.front().Resources.SlotRequirement.ExclusiveExecution);
  OUTJOBS.clear();


  // Test order of attributes
  xrsl = "&(executable = \"/bin/echo\")"
          "(countpernode = 4)"
          "(count = 8)";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(xrsl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(8, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL(4, OUTJOBS.front().Resources.SlotRequirement.SlotsPerHost);
  OUTJOBS.clear();


  // Test failure cases
  xrsl = "&(executable = \"/bin/echo\")"
          "(count = \"eight\")";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, OUTJOBS));
  OUTJOBS.clear();

  xrsl = "&(executable = \"/bin/echo\")"
          "(countpernode = \"four\")";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, OUTJOBS));
  OUTJOBS.clear();

  xrsl = "&(executable = \"/bin/echo\")"
          "(exclusiveexecution = \"yes-thank-you\")";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(xrsl, OUTJOBS));
  OUTJOBS.clear();


  INJOB.Application.Executable.Path = "/bin/echo";
  INJOB.Resources.SlotRequirement.SlotsPerHost = 6;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:xrsl"));
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
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe1", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe2", OUTJOBS.back().Application.Executable.Path);
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
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.1", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.2.1", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.2.2", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q2", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(OUTJOBS.front().UseAlternative());
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q3", OUTJOBS.front().Resources.QueueName);

  CPPUNIT_ASSERT(!OUTJOBS.front().UseAlternative());
  OUTJOBS.front().UseOriginal();
  CPPUNIT_ASSERT_EQUAL(4, (int)OUTJOBS.front().GetAlternatives().size());
  CPPUNIT_ASSERT_EQUAL((std::string)"/bin/exe", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello world!", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"q1.1", OUTJOBS.front().Resources.QueueName);
}

void XRSLParserTest::TestRTE() {
  xrsl = "&(executable=/bin/true)"
         "(runTimeEnvironment=\"RTE-1.2.3\" \"option1\" \"option2\")";

  CPPUNIT_ASSERT(PARSER.Parse(xrsl, OUTJOBS));
  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:xrsl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(tempjobdesc, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().Resources.RunTimeEnvironment.getSoftwareList().size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Resources.RunTimeEnvironment.getSoftwareList().front().getOptions().size());
  CPPUNIT_ASSERT_EQUAL(std::string(""), OUTJOBS.front().Resources.RunTimeEnvironment.getSoftwareList().front().getFamily());
  CPPUNIT_ASSERT_EQUAL(std::string("RTE"), OUTJOBS.front().Resources.RunTimeEnvironment.getSoftwareList().front().getName());
  CPPUNIT_ASSERT_EQUAL(std::string("option1"), OUTJOBS.front().Resources.RunTimeEnvironment.getSoftwareList().front().getOptions().front());
}

CPPUNIT_TEST_SUITE_REGISTRATION(XRSLParserTest);
