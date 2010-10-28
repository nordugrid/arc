#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>

#include "../ARCJSDLParser.h"


#define PARSER parser
#define INJOB inJob
#define OUTJOB outJob
#define MESSAGE message

#define UNPARSE_PARSE OUTJOB = PARSER.Parse(PARSER.UnParse(INJOB));

#define PARSE_ASSERT(X) \
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, X);

#define PARSE_ASSERT_EQUAL(X) \
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.X, OUTJOB.X);


class ARCJSDLParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ARCJSDLParserTest);
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
  ARCJSDLParserTest() {}

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
  Arc::JobDescription INJOB, OUTJOB;
  Arc::ARCJSDLParser PARSER;

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

void ARCJSDLParserTest::setUp() {
  INJOB.Application.Executable.Name = "executable";
  INJOB.Application.Executable.Argument.push_back("arg1");
  INJOB.Application.Executable.Argument.push_back("arg2");
  INJOB.Application.Executable.Argument.push_back("arg3");
}

void ARCJSDLParserTest::tearDown() {
}

void ARCJSDLParserTest::TestExecutable() {
  MESSAGE = "Error parsing executable related attributes.";

  UNPARSE_PARSE;
  PARSE_ASSERT_EQUAL(Application.Executable.Name);
  PARSE_ASSERT_EQUAL(Application.Executable.Argument);
}

void ARCJSDLParserTest::TestInputOutputError() {
  MESSAGE = "Error parsing standard input/output/error attributes.";
  INJOB.Application.Input = "input-file";
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
 * create/delete the file, in total giving 9 options. In the table below the
 * supported cases for ARCJSDL is shown. These are tested in the methods below
 * the table. In addition in the JobDescription the "IsExecutable" and
 * "DownloadToCache" can be specified for a file, these are not fully tested
 * yet.
 *
 *                     T    A    R    G    E    T
 *                ------------------------------------
 *                |  DELETE  |  DOWNLOAD  |  UPLOAD  |
 *  S ------------------------------------------------
 *  O |   CREATE  |    X     |      X     |    X     |
 *  U ------------------------------------------------
 *  R |  DOWNLOAD |    X     |      X     |    X     |
 *  C ------------------------------------------------
 *  E |   UPLOAD  |    X     |      X     |    X     |
 *    ------------------------------------------------
 *
 **/

 /** 1-Create-Delete */
void ARCJSDLParserTest::TestDataStagingCreateDelete() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing create-delete data staging type.";

  Arc::FileType file;
  file.Name = "1-Create-Delete";
  file.KeepData = false;
  file.IsExecutable = false;
  file.DownloadToCache = false;
  INJOB.DataStaging.File.push_back(file);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1 && OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().IsExecutable);

  INJOB.DataStaging.File.back().IsExecutable = true;

  UNPARSE_PARSE;
  PARSE_ASSERT_EQUAL(DataStaging.File.front().IsExecutable);
}

/** 2-Download-Delete */
void ARCJSDLParserTest::TestDataStagingDownloadDelete() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing download-delete data staging type.";

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
  INJOB.DataStaging.File.push_back(file);

  CPPUNIT_ASSERT(INJOB.DataStaging.File.size() == 1);
  CPPUNIT_ASSERT(INJOB.DataStaging.File.front().Source.size() == 1 && INJOB.DataStaging.File.front().Source.front().URI);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Source.size() == 1 && OUTJOB.DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Source.front().URI);
}

/** 3-Upload-Delete */
void ARCJSDLParserTest::TestDataStagingUploadDelete() {
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

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Source.size() == 1 && OUTJOB.DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Source.front().URI);
}

/** 4-Create-Download */
void ARCJSDLParserTest::TestDataStagingCreateDownload() {
  INJOB.DataStaging.File.clear();
  MESSAGE = "Error parsing create-download data staging type.";

  Arc::FileType file;
  file.Name = "4-Create-Download";
  file.KeepData = true;
  INJOB.DataStaging.File.push_back(file);

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().KeepData);
}

/** 5-Download-Download */
void ARCJSDLParserTest::TestDataStagingDownloadDownload() {
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
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Source.size() == 1 && OUTJOB.DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().KeepData);
}

/** 6-Upload-Download */
void ARCJSDLParserTest::TestDataStagingUploadDownload() {
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

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Source.size() == 1 && OUTJOB.DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().KeepData);
}

/** 7-Create-Upload */
void ARCJSDLParserTest::TestDataStagingCreateUpload() {
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
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Target.size() == 1 && OUTJOB.DataStaging.File.front().Target.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Target.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().KeepData);
}

/** 8-Download-Upload */
void ARCJSDLParserTest::TestDataStagingDownloadUpload() {
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
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Source.size() == 1 && OUTJOB.DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Target.size() == 1 && OUTJOB.DataStaging.File.front().Target.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Target.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().KeepData);
}

/** 9-Upload-Upload */
void ARCJSDLParserTest::TestDataStagingUploadUpload() {
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

  UNPARSE_PARSE;
  PARSE_ASSERT(OUTJOB);
  PARSE_ASSERT(OUTJOB.DataStaging.File.size() == 1);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Name);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Source.size() == 1 && OUTJOB.DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Source.front().URI);
  PARSE_ASSERT(OUTJOB.DataStaging.File.front().Target.size() == 1 && OUTJOB.DataStaging.File.front().Target.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().Target.front().URI);
  PARSE_ASSERT_EQUAL(DataStaging.File.front().KeepData);
}

void ARCJSDLParserTest::TestPOSIXCompliance() {
  INJOB.DataStaging.File.clear();
  /** Testing compliance with GFD 56 - the POSIX extension **/
  MESSAGE = "Error: The parser does not comply with the JDSL POSIX specification.";

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

  INJOB.Application.Input = "input";
  INJOB.Application.Output = "output";
  INJOB.Application.Error = "error";

  INJOB.Application.Environment.push_back(std::make_pair("var1", "value1"));
  INJOB.Application.Environment.push_back(std::make_pair("var2", "value2"));
  INJOB.Application.Environment.push_back(std::make_pair("var3", "value3"));

  INJOB.Resources.TotalWallTime = 50;
  INJOB.Resources.IndividualPhysicalMemory = 100;
  INJOB.Resources.TotalCPUTime = 110;
  INJOB.Resources.SlotRequirement.NumberOfSlots = 2;
  INJOB.Resources.IndividualVirtualMemory = 500;
  INJOB.Resources.SlotRequirement.ThreadsPerProcesses = 7;

  OUTJOB = PARSER.Parse(posixJSDLStr);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Executable.Name, OUTJOB.Application.Executable.Name);
  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", INJOB.Application.Executable.Argument == OUTJOB.Application.Executable.Argument);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Input, OUTJOB.Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Output, OUTJOB.Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Error, OUTJOB.Application.Error);
  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", INJOB.Application.Environment == OUTJOB.Application.Environment);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.TotalWallTime.range.max, OUTJOB.Resources.TotalWallTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualPhysicalMemory.max, OUTJOB.Resources.IndividualPhysicalMemory.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.TotalCPUTime.range.max, OUTJOB.Resources.TotalCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.SlotRequirement.NumberOfSlots.max, OUTJOB.Resources.SlotRequirement.NumberOfSlots.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualVirtualMemory.max, OUTJOB.Resources.IndividualVirtualMemory.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.SlotRequirement.ThreadsPerProcesses.max, OUTJOB.Resources.SlotRequirement.ThreadsPerProcesses.max);

  Arc::XMLNode xmlArcJSDL(PARSER.UnParse(OUTJOB));
  Arc::XMLNode pApp = xmlArcJSDL["JobDescription"]["Application"]["POSIXApplication"];

  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", pApp);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Executable.Name, (std::string)pApp["Executable"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"arg1", (std::string)pApp["Argument"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"arg2", (std::string)pApp["Argument"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"arg3", (std::string)pApp["Argument"][2]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Input, (std::string)pApp["Input"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Output, (std::string)pApp["Output"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Error, (std::string)pApp["Error"]);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"var1",   (std::string)pApp["Environment"][0].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"value1", (std::string)pApp["Environment"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"var2",   (std::string)pApp["Environment"][1].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"value2", (std::string)pApp["Environment"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"var3",   (std::string)pApp["Environment"][2].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", (std::string)"value3", (std::string)pApp["Environment"][2]);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.TotalWallTime.range.max, Arc::stringto<int>(pApp["WallTimeLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualPhysicalMemory.max, Arc::stringto<int64_t>(pApp["MemoryLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.TotalCPUTime.range.max, Arc::stringto<int>(pApp["CPUTimeLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.SlotRequirement.NumberOfSlots.max, Arc::stringto<int>(pApp["ProcessCountLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualVirtualMemory.max, Arc::stringto<int64_t>(pApp["VirtualMemoryLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.SlotRequirement.ThreadsPerProcesses.max, Arc::stringto<int>(pApp["ThreadCountLimit"]));
}

void ARCJSDLParserTest::TestHPCCompliance() {
  INJOB.DataStaging.File.clear();
  /** Testing compliance with GFD 111 **/
  MESSAGE = "Error: The parser does not comply with the JSDL HPC Profile Application Extension specification.";

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

  INJOB.Application.Input = "input";
  INJOB.Application.Output = "output";
  INJOB.Application.Error = "error";

  INJOB.Application.Environment.push_back(std::make_pair("var1", "value1"));
  INJOB.Application.Environment.push_back(std::make_pair("var2", "value2"));
  INJOB.Application.Environment.push_back(std::make_pair("var3", "value3"));

  OUTJOB = PARSER.Parse(hpcJSDLStr);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Executable.Name, OUTJOB.Application.Executable.Name);
  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", INJOB.Application.Executable.Argument == OUTJOB.Application.Executable.Argument);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Input, OUTJOB.Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Output, OUTJOB.Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Error, OUTJOB.Application.Error);
  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", INJOB.Application.Environment == OUTJOB.Application.Environment);

  Arc::XMLNode xmlArcJSDL(PARSER.UnParse(OUTJOB));
  Arc::XMLNode pApp = xmlArcJSDL["JobDescription"]["Application"]["HPCProfileApplication"];

  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", pApp);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Executable.Name, (std::string)pApp["Executable"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"arg1", (std::string)pApp["Argument"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"arg2", (std::string)pApp["Argument"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"arg3", (std::string)pApp["Argument"][2]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Input,  (std::string)pApp["Input"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Output, (std::string)pApp["Output"]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Error,  (std::string)pApp["Error"]);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"var1",   (std::string)pApp["Environment"][0].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"value1", (std::string)pApp["Environment"][0]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"var2",   (std::string)pApp["Environment"][1].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"value2", (std::string)pApp["Environment"][1]);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"var3",   (std::string)pApp["Environment"][2].Attribute("name"));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", (std::string)"value3", (std::string)pApp["Environment"][2]);
}

CPPUNIT_TEST_SUITE_REGISTRATION(ARCJSDLParserTest);
