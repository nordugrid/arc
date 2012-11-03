#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>
#include <arc/compute/JobDescription.h>

#include "../JDLParser.h"


class JDLParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(JDLParserTest);
  CPPUNIT_TEST(TestExecutable);
  CPPUNIT_TEST(TestInputOutputError);
  CPPUNIT_TEST(TestInputFileClientStageable);
  CPPUNIT_TEST(TestInputFileServiceStageable);
  CPPUNIT_TEST(TestOutputFileClientStageable);
  CPPUNIT_TEST(TestOutputFileServiceStageable);
  CPPUNIT_TEST(TestQueue);
  CPPUNIT_TEST(TestAdditionalAttributes);
  CPPUNIT_TEST_SUITE_END();

public:
  JDLParserTest():PARSER((Arc::PluginArgument*)NULL) {}

  void setUp();
  void tearDown();
  void TestExecutable();
  void TestInputOutputError();
  void TestInputFileClientStageable();
  void TestInputFileServiceStageable();
  void TestOutputFileClientStageable();
  void TestOutputFileServiceStageable();
  void TestQueue();
  void TestAdditionalAttributes();

private:
  Arc::JobDescription INJOB;
  std::list<Arc::JobDescription> OUTJOBS;
  Arc::JDLParser PARSER;

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

void JDLParserTest::setUp() {
  MESSAGE = " ";
  INJOB.Application.Executable.Path = "executable";
  INJOB.Application.Executable.Argument.push_back("arg1");
  INJOB.Application.Executable.Argument.push_back("arg2");
  INJOB.Application.Executable.Argument.push_back("arg3");
}

void JDLParserTest::tearDown() {
}

void JDLParserTest::TestExecutable() {
  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "egee:jdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Executable.Path, OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Executable.Argument, OUTJOBS.front().Application.Executable.Argument);
}

void JDLParserTest::TestInputOutputError() {
  INJOB.Application.Input = "input-file";
  INJOB.Application.Output = "output-file";
  INJOB.Application.Error = "error-file";

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "egee:jdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());


  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Input, OUTJOBS.front().Application.Input);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Output, OUTJOBS.front().Application.Output);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, INJOB.Application.Error, OUTJOBS.front().Application.Error);
}

/** Client stageable input file */
void JDLParserTest::TestInputFileClientStageable() {
  INJOB.DataStaging.InputFiles.clear();
  MESSAGE = "Error parsing TestInputFileClientStageable data staging type.";

  Arc::InputFileType file;
  file.Name = "TestInputFileClientStageable";
  file.Sources.push_back(Arc::URL(file.Name));
  INJOB.DataStaging.InputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "egee:jdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2, (int)OUTJOBS.front().DataStaging.InputFiles.size());

  std::list<Arc::InputFileType>::const_iterator it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name, it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Sources.back(), it->Sources.front());

  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable", it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::SourceType("executable"), it->Sources.front());
}

/** Service stageable input file */
void JDLParserTest::TestInputFileServiceStageable() {
  INJOB.DataStaging.InputFiles.clear();
  MESSAGE = "Error parsing TestInputFileServiceStageable data staging type.";

  Arc::InputFileType file;
  file.Name = "TestInputFileServiceStageable";
  file.Sources.push_back(Arc::URL("http://example.com/" + file.Name));
  file.IsExecutable = false;
  INJOB.DataStaging.InputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "egee:jdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 2, (int)OUTJOBS.front().DataStaging.InputFiles.size());

  std::list<Arc::InputFileType>::const_iterator it = OUTJOBS.front().DataStaging.InputFiles.begin();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name,            it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.IsExecutable,    it->IsExecutable);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Sources.back(), it->Sources.front());

  it++;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable", it->Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)it->Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::SourceType("executable"), it->Sources.front());
}

/** Client stageable output file */
void JDLParserTest::TestOutputFileClientStageable() {
  INJOB.DataStaging.InputFiles.clear();
  INJOB.DataStaging.OutputFiles.clear();
  MESSAGE = "Error parsing TestOutputFileClientStageable data staging type.";

  Arc::OutputFileType file;
  file.Name = "TestOutputFileClientStageable";
  INJOB.DataStaging.OutputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "egee:jdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  Arc::InputFileType& ifile = OUTJOBS.front().DataStaging.InputFiles.front();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable", ifile.Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)ifile.Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::SourceType("executable"), ifile.Sources.front());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.front().DataStaging.OutputFiles.size());
  Arc::OutputFileType& ofile = OUTJOBS.front().DataStaging.OutputFiles.front();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name, ofile.Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 0, (int)ofile.Targets.size());
}

/** Service stageable output file */
void JDLParserTest::TestOutputFileServiceStageable() {
  INJOB.DataStaging.InputFiles.clear();
  INJOB.DataStaging.OutputFiles.clear();
  MESSAGE = "Error parsing TestOutputFileServiceStageable data staging type.";

  Arc::OutputFileType file;
  file.Name = "TestOutputFileServiceStageable";
  file.Targets.push_back(Arc::URL("http://example.com/" + file.Name));
  INJOB.DataStaging.OutputFiles.push_back(file);

  std::string tempjobdesc;
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.UnParse(INJOB, tempjobdesc, "egee:jdl"));
  CPPUNIT_ASSERT_MESSAGE(MESSAGE, PARSER.Parse(tempjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.front().DataStaging.InputFiles.size());
  Arc::InputFileType& ifile = OUTJOBS.front().DataStaging.InputFiles.front();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, (std::string)"executable", ifile.Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)ifile.Sources.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, Arc::SourceType("executable"), ifile.Sources.front());

  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)OUTJOBS.front().DataStaging.OutputFiles.size());
  Arc::OutputFileType& ofile = OUTJOBS.front().DataStaging.OutputFiles.front();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Name, ofile.Name);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, 1, (int)ofile.Targets.size());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(MESSAGE, file.Targets.back(), ofile.Targets.front());
}

void JDLParserTest::TestQueue() {
  std::string jdl = "["
"Executable = \"executable\";"
"QueueName = \"q1\";"
"]";

  CPPUNIT_ASSERT(PARSER.Parse(jdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"q1", OUTJOBS.front().Resources.QueueName);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());

  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), jdl, "egee:jdl"));
  CPPUNIT_ASSERT(PARSER.Parse(jdl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"q1", OUTJOBS.front().Resources.QueueName);
  CPPUNIT_ASSERT_EQUAL(0, (int)OUTJOBS.front().GetAlternatives().size());
}

void JDLParserTest::TestAdditionalAttributes() {
  std::string tmpjobdesc;

  INJOB.OtherAttributes["egee:jdl;batchsystem"] = "test";
  INJOB.OtherAttributes["egee:jdl;unknownattribute"] = "none";
  INJOB.OtherAttributes["bogus:nonexisting;foo"] = "bar";
  CPPUNIT_ASSERT(PARSER.UnParse(INJOB, tmpjobdesc, "egee:jdl"));
  CPPUNIT_ASSERT(PARSER.Parse(tmpjobdesc, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  std::map<std::string, std::string>::const_iterator itAttribute;
  itAttribute = OUTJOBS.front().OtherAttributes.find("egee:jdl;batchsystem");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() != itAttribute);
  CPPUNIT_ASSERT_EQUAL((std::string)"test", itAttribute->second);
  itAttribute = OUTJOBS.front().OtherAttributes.find("egee:jdl;unknownattribute");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() == itAttribute);
  itAttribute = OUTJOBS.front().OtherAttributes.find("bogus:nonexisting;foo");
  CPPUNIT_ASSERT(OUTJOBS.front().OtherAttributes.end() == itAttribute);
}
CPPUNIT_TEST_SUITE_REGISTRATION(JDLParserTest);
