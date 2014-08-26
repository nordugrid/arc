#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <list>
std::ostream& operator<<(std::ostream& os, const std::list<std::string>& strings);

#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/compute/JobDescription.h>

#include "../ARCJSDLParser.h"


class ARCJSDLParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ARCJSDLParserTest);
  CPPUNIT_TEST(TestExecutable);
  CPPUNIT_TEST(TestInputOutputError);
  CPPUNIT_TEST(TestInputFileClientStageable);
  CPPUNIT_TEST(TestInputFileServiceStageable);
  CPPUNIT_TEST(TestOutputFileClientStageable);
  CPPUNIT_TEST(TestOutputFileServiceStageable);
  CPPUNIT_TEST(TestURIOptionsInput);
  CPPUNIT_TEST(TestURIOptionsOutput);
  CPPUNIT_TEST(TestQueue);
  CPPUNIT_TEST(TestDryRun);
  CPPUNIT_TEST(TestAccessControl);
  CPPUNIT_TEST(TestBasicJSDLCompliance);
  CPPUNIT_TEST(TestPOSIXCompliance);
  CPPUNIT_TEST(TestHPCCompliance);
  CPPUNIT_TEST(TestRangeValueType);
  CPPUNIT_TEST_SUITE_END();

public:
  ARCJSDLParserTest():PARSER((Arc::PluginArgument*)NULL) {}

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
  void TestQueue();
  void TestDryRun();
  void TestAccessControl();
  void TestBasicJSDLCompliance();
  void TestPOSIXCompliance();
  void TestHPCCompliance();
  void TestRangeValueType();

private:
  Arc::JobDescription INJOB;
  std::list<Arc::JobDescription> OUTJOBS;
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
  INJOB.Application.Executable.Path = "executable";
  INJOB.Application.Executable.Argument.push_back("arg1");
  INJOB.Application.Executable.Argument.push_back("arg2");
  INJOB.Application.Executable.Argument.push_back("arg3");
}

void ARCJSDLParserTest::tearDown() {
}

void ARCJSDLParserTest::TestExecutable() {
  MESSAGE = "Error parsing executable related attributes.";

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:jsdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Executable.Path, OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Executable.Argument, OUTJOBS.front().Application.Executable.Argument);
}

void ARCJSDLParserTest::TestInputOutputError() {
  MESSAGE = "Error parsing standard input/output/error attributes.";
  INJOB.Application.Input = "input-file";
  INJOB.Application.Output = "output-file";
  INJOB.Application.Error = "error-file";

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:jsdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Input, OUTJOBS.front().Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Output, OUTJOBS.front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Error, OUTJOBS.front().Application.Error);
}

/** Client stageable input file */
void ARCJSDLParserTest::TestInputFileClientStageable() {
  INJOB.DataStaging.InputFiles.clear();
  MESSAGE = "Error parsing TestInputFileClientStageable data staging type.";

  Arc::InputFileType file;
  file.Name = "TestInputFileClientStageable";
  file.Sources.push_back(Arc::URL(file.Name));
  INJOB.DataStaging.InputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:jsdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().DataStaging.InputFiles.size() == 1);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.DataStaging.InputFiles.front().Name, OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().DataStaging.InputFiles.front().Sources.size() == 1 && OUTJOBS.front().DataStaging.InputFiles.front().Sources.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.DataStaging.InputFiles.front().Sources.front(), OUTJOBS.front().DataStaging.InputFiles.front().Sources.front());
}

/** Service stageable input file */
void ARCJSDLParserTest::TestInputFileServiceStageable() {
  INJOB.DataStaging.InputFiles.clear();
  MESSAGE = "Error parsing TestInputFileServiceStageable data staging type.";

  Arc::InputFileType file;
  file.Name = "TestInputFileServiceStageable";
  file.Sources.push_back(Arc::URL("http://example.com/" + file.Name));
  file.IsExecutable = false;
  INJOB.DataStaging.InputFiles.push_back(file);

  CPPUNIT_ASSERT(INJOB.DataStaging.InputFiles.size() == 1);
  CPPUNIT_ASSERT(INJOB.DataStaging.InputFiles.front().Sources.size() == 1 && INJOB.DataStaging.InputFiles.front().Sources.front());

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:jsdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().DataStaging.InputFiles.size() == 1);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.DataStaging.InputFiles.front().Name, OUTJOBS.front().DataStaging.InputFiles.front().Name);
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().DataStaging.InputFiles.front().Sources.size() == 1 && OUTJOBS.front().DataStaging.InputFiles.front().Sources.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.DataStaging.InputFiles.front().Sources.front(), OUTJOBS.front().DataStaging.InputFiles.front().Sources.front());
}

/** Client stageable output file */
void ARCJSDLParserTest::TestOutputFileClientStageable() {
  INJOB.DataStaging.OutputFiles.clear();
  MESSAGE = "Error parsing TestOutputFileClientStageable data staging type.";

  Arc::OutputFileType file;
  file.Name = "TestOutputFileClientStageable";
  INJOB.DataStaging.OutputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:jsdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().DataStaging.OutputFiles.size() == 1);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.DataStaging.OutputFiles.front().Name, OUTJOBS.front().DataStaging.OutputFiles.front().Name);
}

/** Service stageable output file */
void ARCJSDLParserTest::TestOutputFileServiceStageable() {
  INJOB.DataStaging.OutputFiles.clear();
  MESSAGE = "Error parsing TestOutputFileServiceStageable data staging type.";

  Arc::OutputFileType file;
  file.Name = "TestOutputFileServiceStageable";
  file.Targets.push_back(Arc::URL("http://example.com/" + file.Name));
  INJOB.DataStaging.OutputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "nordugrid:jsdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().DataStaging.OutputFiles.size() == 1);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.DataStaging.OutputFiles.front().Name, OUTJOBS.front().DataStaging.OutputFiles.front().Name);
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, OUTJOBS.front().DataStaging.OutputFiles.front().Targets.size() == 1 && OUTJOBS.front().DataStaging.OutputFiles.front().Targets.front());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.DataStaging.OutputFiles.front().Targets.front(), OUTJOBS.front().DataStaging.OutputFiles.front().Targets.front());
}

void ARCJSDLParserTest::TestURIOptionsInput() {
  std::string jsdl = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:posix-jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\">"
  "<JobDescription>"
    "<Application>"
      "<posix-jsdl:POSIXApplication>"
      "<posix-jsdl:Executable>executable</posix-jsdl:Executable>"
      "</posix-jsdl:POSIXApplication>"
    "</Application>"
    "<DataStaging>"
      "<FileName>test.file</FileName>"
      "<Source>"
        "<URI>gsiftp://example.com/test.file</URI>"
        "<URIOption>threads=5</URIOption>"
      "</Source>"
    "</DataStaging>"
  "</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  const Arc::InputFileType f(OUTJOBS.front().DataStaging.InputFiles.front());
  CPPUNIT_ASSERT_EQUAL(std::string("test.file"), f.Name);
  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://example.com:2811/test.file"), f.Sources.front().str());
  CPPUNIT_ASSERT_EQUAL(std::string("5"), f.Sources.front().Option("threads"));
}

void ARCJSDLParserTest::TestURIOptionsOutput() {
  std::string jsdl = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:posix-jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\">"
  "<JobDescription>"
    "<Application>"
      "<posix-jsdl:POSIXApplication>"
      "<posix-jsdl:Executable>executable</posix-jsdl:Executable>"
      "</posix-jsdl:POSIXApplication>"
    "</Application>"
    "<DataStaging>"
      "<FileName>test.file</FileName>"
      "<Target>"
        "<URI>lfc://example.com/test.file</URI>"
        "<URIOption>checksum=md5</URIOption>"
        "<Location>"
          "<URI>gsiftp://example.com/test.file</URI>"
          "<URIOption>threads=5</URIOption>"
        "</Location>"
        "<Location>"
          "<URI>gsiftp://example2.com/test.file</URI>"
          "<URIOption>threads=10</URIOption>"
        "</Location>"
      "</Target>"
    "</DataStaging>"
  "</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.front().DataStaging.OutputFiles.size());
  const Arc::OutputFileType f(OUTJOBS.front().DataStaging.OutputFiles.front());

  CPPUNIT_ASSERT_EQUAL(std::string("test.file"), f.Name);
  CPPUNIT_ASSERT_EQUAL(std::string("lfc://example.com:5010/test.file"), f.Targets.front().str());
  CPPUNIT_ASSERT_EQUAL(std::string("md5"), f.Targets.front().Option("checksum"));

  const std::list<Arc::URLLocation> locations(f.Targets.front().Locations());
  CPPUNIT_ASSERT_EQUAL(2, (int)locations.size());

  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://example.com:2811/test.file"), locations.front().str());
  CPPUNIT_ASSERT_EQUAL(std::string("5"), locations.front().Option("threads"));

  CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://example2.com:2811/test.file"), locations.back().str());
  CPPUNIT_ASSERT_EQUAL(std::string("10"), locations.back().Option("threads"));
}

void ARCJSDLParserTest::TestQueue() {
  std::string jsdl = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:posix-jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\">"
"<JobDescription>"
"<Application>"
"<posix-jsdl:POSIXApplication>"
"<posix-jsdl:Executable>executable</posix-jsdl:Executable>"
"</posix-jsdl:POSIXApplication>"
"</Application>"
"<Resources>"
"<QueueName>q1</QueueName>"
"</Resources>"
"</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"q1", OUTJOBS.front().Resources.QueueName);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), jsdl, "nordugrid:jsdl"));
  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"q1", OUTJOBS.front().Resources.QueueName);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  jsdl = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:posix-jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\">"
"<JobDescription>"
"<Application>"
"<posix-jsdl:POSIXApplication>"
"<posix-jsdl:Executable>executable</posix-jsdl:Executable>"
"</posix-jsdl:POSIXApplication>"
"</Application>"
"<Resources>"
"<QueueName require=\"ne\">q1</QueueName>"
"</Resources>"
"</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  std::map<std::string, std::string>::const_iterator itAttribute;
  itAttribute = OUTJOBS.front().OtherAttributes.find("nordugrid:broker;reject_queue");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() != itAttribute);
  CPPUNIT_ASSERT_EQUAL((std::string)"q1", itAttribute->second);

  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), jsdl, "nordugrid:jsdl"));
  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  itAttribute = OUTJOBS.front().OtherAttributes.find("nordugrid:broker;reject_queue");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() == itAttribute);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());
}

void ARCJSDLParserTest::TestDryRun() {
  std::string jsdl = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\">"
"<JobDescription>"
"<Application>"
"<Executable>"
"<Path>executable</Path>"
"</Executable>"
"<DryRun>yes</DryRun>"
"</Application>"
"</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(OUTJOBS.front().Application.DryRun);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), jsdl, "nordugrid:jsdl"));
  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(OUTJOBS.front().Application.DryRun);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  jsdl = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\">"
"<JobDescription>"
"<Application>"
"<Executable>"
"<Path>executable</Path>"
"</Executable>"
"<DryRun>no</DryRun>"
"</Application>"
"</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(!OUTJOBS.front().Application.DryRun);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), jsdl, "nordugrid:jsdl"));
  CPPUNIT_ASSERT(PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(!OUTJOBS.front().Application.DryRun);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());
}

void ARCJSDLParserTest::TestAccessControl() {
  std::string jsdl = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\">"
"<JobDescription>"
"<Application>"
"<Executable>"
"<Path>executable</Path>"
"</Executable>"
"<AccessControl>"
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
"</gacl>"
"</AccessControl>"
"</Application>"
"</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(jsdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(OUTJOBS.front().Application.AccessControl);
  CPPUNIT_ASSERT_EQUAL((std::string)"gacl", OUTJOBS.front().Application.AccessControl.Name());
  CPPUNIT_ASSERT_EQUAL(1, OUTJOBS.front().Application.AccessControl.Size());
  CPPUNIT_ASSERT_EQUAL((std::string)"entry", OUTJOBS.front().Application.AccessControl.Child().Name());

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:jsdl"));
  OUTJOBS.clear();

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT(OUTJOBS.front().Application.AccessControl);
  CPPUNIT_ASSERT_EQUAL((std::string)"gacl", OUTJOBS.front().Application.AccessControl.Name());
  CPPUNIT_ASSERT_EQUAL(1, OUTJOBS.front().Application.AccessControl.Size());
  CPPUNIT_ASSERT_EQUAL((std::string)"entry", OUTJOBS.front().Application.AccessControl.Child().Name());
}

void ARCJSDLParserTest::TestBasicJSDLCompliance() {
  /** Testing compliance with GFD 56 **/
  MESSAGE = "Error: The parser does not comply with the JDSL specification.";

  const std::string jsdlStr = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:posix-jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\">"
"<JobDescription>"
"<Application>"
"<posix-jsdl:POSIXApplication>"
"<posix-jsdl:Executable>executable</posix-jsdl:Executable>"
"</posix-jsdl:POSIXApplication>"
"</Application>"
"<Resources>"
"<FileSystem>"
"<DiskSpace>"
"<LowerBoundedRange>128974848</LowerBoundedRange>" // 1024*1024*123
"</DiskSpace>"
"</FileSystem>"
"</Resources>"
"</JobDescription>"
"</JobDefinition>";

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(jsdlStr, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 123, OUTJOBS.front().Resources.DiskSpaceRequirement.DiskSpace.min);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:jsdl"));

  Arc::XMLNode xmlArcJSDL(tempjobdesc);
}

void ARCJSDLParserTest::TestPOSIXCompliance() {
  /** Testing compliance with GFD 56 - the POSIX extension **/
  MESSAGE = "Error: The parser does not comply with the JDSL POSIX specification.";

  const std::string posixJSDLStr = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:posix-jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\">"
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
"<posix-jsdl:MemoryLimit>104857600</posix-jsdl:MemoryLimit>"
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
  INJOB.Resources.ParallelEnvironment.ThreadsPerProcess = 7;

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(posixJSDLStr, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Executable.Path, OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", INJOB.Application.Executable.Argument == OUTJOBS.front().Application.Executable.Argument);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Input, OUTJOBS.front().Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Output, OUTJOBS.front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Error, OUTJOBS.front().Application.Error);
  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", INJOB.Application.Environment == OUTJOBS.front().Application.Environment);

  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.TotalWallTime.range.max, OUTJOBS.front().Resources.TotalWallTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualPhysicalMemory.max, OUTJOBS.front().Resources.IndividualPhysicalMemory.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.TotalCPUTime.range.max, OUTJOBS.front().Resources.TotalCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.SlotRequirement.NumberOfSlots, OUTJOBS.front().Resources.SlotRequirement.NumberOfSlots);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualVirtualMemory.max, OUTJOBS.front().Resources.IndividualVirtualMemory.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.ParallelEnvironment.ThreadsPerProcess, OUTJOBS.front().Resources.ParallelEnvironment.ThreadsPerProcess);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:jsdl"));

  Arc::XMLNode xmlArcJSDL(tempjobdesc);
  Arc::XMLNode pApp = xmlArcJSDL["JobDescription"]["Application"]["POSIXApplication"];

  CPPUNIT_ASSERT_MESSAGE("POSIX compliance failure", pApp);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Application.Executable.Path, (std::string)pApp["Executable"]);
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
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualPhysicalMemory.max, (int)(Arc::stringto<long long>(pApp["MemoryLimit"])/(1024*1024)));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.TotalCPUTime.range.max, Arc::stringto<int>(pApp["CPUTimeLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.SlotRequirement.NumberOfSlots, Arc::stringto<int>(pApp["ProcessCountLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.IndividualVirtualMemory.max, Arc::stringto<int>(pApp["VirtualMemoryLimit"]));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("POSIX compliance failure", INJOB.Resources.ParallelEnvironment.ThreadsPerProcess, Arc::stringto<int>(pApp["ThreadCountLimit"]));
}

void ARCJSDLParserTest::TestHPCCompliance() {
  /** Testing compliance with GFD 111 **/
  MESSAGE = "Error: The parser does not comply with the JSDL HPC Profile Application Extension specification.";

  const std::string hpcJSDLStr = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:HPC-jsdl=\"http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa\">"
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

  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(hpcJSDLStr, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Executable.Path, OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", INJOB.Application.Executable.Argument == OUTJOBS.front().Application.Executable.Argument);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Input, OUTJOBS.front().Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Output, OUTJOBS.front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Error, OUTJOBS.front().Application.Error);
  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", INJOB.Application.Environment == OUTJOBS.front().Application.Environment);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "nordugrid:jsdl"));

  Arc::XMLNode xmlArcJSDL(tempjobdesc);
  Arc::XMLNode pApp = xmlArcJSDL["JobDescription"]["Application"]["HPCProfileApplication"];

  CPPUNIT_ASSERT_MESSAGE("HPC compliance failure", pApp);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("HPC compliance failure", INJOB.Application.Executable.Path, (std::string)pApp["Executable"]);
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

void ARCJSDLParserTest::TestRangeValueType() {
  /** Testing compliance with the RangeValue_Type **/
  MESSAGE = "Error: The parser does not comply with the JSDL RangeValue_Type type.";

  const std::string beforeElement = "<?xml version=\"1.0\"?>"
"<JobDefinition xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\""
" xmlns:posix-jsdl=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\"><JobDescription>"
"<Application><posix-jsdl:POSIXApplication><posix-jsdl:Executable>executable</posix-jsdl:Executable></posix-jsdl:POSIXApplication></Application>"
"<Resources><IndividualCPUTime>";
  const std::string afterElement ="</IndividualCPUTime></Resources></JobDescription></JobDefinition>";

  std::string element;

  element = "<Exact>3600.3</Exact>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 3600, OUTJOBS.front().Resources.IndividualCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, -1, OUTJOBS.front().Resources.IndividualCPUTime.range.min);

  element = "<LowerBoundedRange>134.5</LowerBoundedRange>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, -1, OUTJOBS.front().Resources.IndividualCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 134, OUTJOBS.front().Resources.IndividualCPUTime.range.min);

  element = "<UpperBoundedRange>234.5</UpperBoundedRange>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 234, OUTJOBS.front().Resources.IndividualCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, -1, OUTJOBS.front().Resources.IndividualCPUTime.range.min);

  element = "<UpperBoundedRange>234.5</UpperBoundedRange><LowerBoundedRange>123.4</LowerBoundedRange>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 234, OUTJOBS.front().Resources.IndividualCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 123, OUTJOBS.front().Resources.IndividualCPUTime.range.min);

  element = "<Range><UpperBound>234.5</UpperBound><LowerBound>123.4</LowerBound></Range>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 234, OUTJOBS.front().Resources.IndividualCPUTime.range.max);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 123, OUTJOBS.front().Resources.IndividualCPUTime.range.min);

  element = "<Range><UpperBound>123.4</UpperBound><LowerBound>234.5</LowerBound></Range>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));

  element = "<Range><UpperBound>234.5</UpperBound></Range><Range><LowerBound>123.4</LowerBound></Range>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));

  element = "<Exact>234.5</Exact><Exact>123.4</Exact>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));

  element = "<Exact epsilon=\"1.2\">234.5</Exact>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));

  element = "<LowerBoundedRange exclusiveBound=\"1.2\">234.5</LowerBoundedRange>";
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, !PARSER.Parse(beforeElement + element + afterElement, OUTJOBS));
}

CPPUNIT_TEST_SUITE_REGISTRATION(ARCJSDLParserTest);
