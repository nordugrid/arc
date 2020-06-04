#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/compute/JobDescription.h>

#include "../ADLParser.h"


class ADLParserTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ADLParserTest);
  CPPUNIT_TEST(JobNameTest);
  CPPUNIT_TEST(DescriptionTest);
  CPPUNIT_TEST(TypeTest);
  CPPUNIT_TEST(AnnotationTest);
  CPPUNIT_TEST(ActivityOldIDTest);
  CPPUNIT_TEST(ExecutableTest);
  CPPUNIT_TEST(InputTest);
  CPPUNIT_TEST(OutputTest);
  CPPUNIT_TEST(ErrorTest);
  CPPUNIT_TEST(PreExecutableTest);
  CPPUNIT_TEST(PostExecutableTest);
  CPPUNIT_TEST(LoggingDirectoryTest);
  CPPUNIT_TEST(RemoteLoggingTest);
  CPPUNIT_TEST(TestInputFileClientStageable);
  CPPUNIT_TEST(TestInputFileServiceStageable);
  CPPUNIT_TEST(TestOutputFileClientStageable);
  CPPUNIT_TEST(TestOutputFileServiceStageable);
  CPPUNIT_TEST(TestOutputFileLocationsServiceStageable);
  CPPUNIT_TEST_SUITE_END();

public:
  ADLParserTest():PARSER((Arc::PluginArgument*)NULL) {}

  void setUp();
  void tearDown();
  void JobNameTest();
  void DescriptionTest();
  void TypeTest();
  void AnnotationTest();
  void ActivityOldIDTest();
  void ExecutableTest();
  void InputTest();
  void OutputTest();
  void ErrorTest();
  void PreExecutableTest();
  void PostExecutableTest();
  void LoggingDirectoryTest();
  void RemoteLoggingTest();
  void TestInputFileClientStageable();
  void TestInputFileServiceStageable();
  void TestOutputFileClientStageable();
  void TestOutputFileServiceStageable();
  void TestOutputFileLocationsServiceStageable();


private:
  Arc::JobDescription INJOB;
  std::list<Arc::JobDescription> OUTJOBS;
  Arc::ADLParser PARSER;
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

void ADLParserTest::setUp() {
}

void ADLParserTest::tearDown() {
}

void ADLParserTest::JobNameTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:ActivityIdentification>"
"<adl:Name>EMI-ADL-minimal</adl:Name>"
"</adl:ActivityIdentification>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"EMI-ADL-minimal", OUTJOBS.front().Identification.JobName);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"EMI-ADL-minimal", OUTJOBS.front().Identification.JobName);
}

void ADLParserTest::DescriptionTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:ActivityIdentification>"
"<adl:Description>This job description provides a full example of EMI ADL</adl:Description>"
"</adl:ActivityIdentification>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"This job description provides a full example of EMI ADL", OUTJOBS.front().Identification.Description);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"This job description provides a full example of EMI ADL", OUTJOBS.front().Identification.Description);
}

void ADLParserTest::TypeTest() {
  // The job type is parsed into a std::string object, so it is not necessary to test the other values in the Type enumeration.

  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:ActivityIdentification>"
"<adl:Type>single</adl:Type>"
"</adl:ActivityIdentification>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"single", OUTJOBS.front().Identification.Type);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"single", OUTJOBS.front().Identification.Type);
}

void ADLParserTest::AnnotationTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:ActivityIdentification>"
"<adl:Annotation>Full example</adl:Annotation>"
"<adl:Annotation>EMI ADL v. 1.04</adl:Annotation>"
"</adl:ActivityIdentification>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Identification.Annotation.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Full example", OUTJOBS.front().Identification.Annotation.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"EMI ADL v. 1.04", OUTJOBS.front().Identification.Annotation.back());

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Identification.Annotation.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Full example", OUTJOBS.front().Identification.Annotation.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"EMI ADL v. 1.04", OUTJOBS.front().Identification.Annotation.back());
}

void ADLParserTest::ActivityOldIDTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:ActivityIdentification>"
"<nordugrid-adl:ActivityOldID>https://eu-emi.eu/emies/123456789first</nordugrid-adl:ActivityOldID>"
"<nordugrid-adl:ActivityOldID>https://eu-emi.eu/emies/123456789second</nordugrid-adl:ActivityOldID>"
"</adl:ActivityIdentification>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Identification.ActivityOldID.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://eu-emi.eu/emies/123456789first", OUTJOBS.front().Identification.ActivityOldID.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://eu-emi.eu/emies/123456789second", OUTJOBS.front().Identification.ActivityOldID.back());

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Identification.ActivityOldID.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://eu-emi.eu/emies/123456789first", OUTJOBS.front().Identification.ActivityOldID.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"https://eu-emi.eu/emies/123456789second", OUTJOBS.front().Identification.ActivityOldID.back());
}

void ADLParserTest::ExecutableTest() {
  // Try empty executable
  {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));
  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"", OUTJOBS.front().Application.Executable.Path);
  }
  OUTJOBS.clear();

  {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Executable>"
"<adl:Path>my-executable</adl:Path>"
"<adl:Argument>Hello</adl:Argument>"
"<adl:Argument>World</adl:Argument>"
"<adl:FailIfExitCodeNotEqualTo>104</adl:FailIfExitCodeNotEqualTo>"
"</adl:Executable>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"my-executable", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL((std::string)"my-executable", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"World", OUTJOBS.front().Application.Executable.Argument.back());
  CPPUNIT_ASSERT(OUTJOBS.front().Application.Executable.SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(104, OUTJOBS.front().Application.Executable.SuccessExitCode.second);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"my-executable", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL((std::string)"my-executable", OUTJOBS.front().Application.Executable.Path);
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Application.Executable.Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"Hello", OUTJOBS.front().Application.Executable.Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"World", OUTJOBS.front().Application.Executable.Argument.back());
  CPPUNIT_ASSERT(OUTJOBS.front().Application.Executable.SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(104, OUTJOBS.front().Application.Executable.SuccessExitCode.second);
  }

  OUTJOBS.clear();

  // Check if first member of SuccessExitCode is set to false.
  {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Executable>"
"<adl:Path>my-executable</adl:Path>"
"<adl:Argument>Hello</adl:Argument>"
"<adl:Argument>World</adl:Argument>"
"</adl:Executable>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT(!OUTJOBS.front().Application.Executable.SuccessExitCode.first);
  }
}

void ADLParserTest::InputTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Input>standard-emi-input</adl:Input>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"standard-emi-input", OUTJOBS.front().Application.Input);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"standard-emi-input", OUTJOBS.front().Application.Input);
}

void ADLParserTest::OutputTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Output>standard-emi-output</adl:Output>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"standard-emi-output", OUTJOBS.front().Application.Output);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"standard-emi-output", OUTJOBS.front().Application.Output);
}

void ADLParserTest::ErrorTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Error>standard-emi-error</adl:Error>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"standard-emi-error", OUTJOBS.front().Application.Error);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"standard-emi-error", OUTJOBS.front().Application.Error);
}

void ADLParserTest::PreExecutableTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:PreExecutable>"
"<adl:Path>my-first-pre-executable</adl:Path>"
"<adl:Argument>123456789</adl:Argument>"
"<adl:Argument>xyz</adl:Argument>"
"<adl:FailIfExitCodeNotEqualTo>0</adl:FailIfExitCodeNotEqualTo>"
"</adl:PreExecutable>"
"<adl:PreExecutable>"
"<adl:Path>foo/my-second-pre-executable</adl:Path>"
"<adl:Argument>1357924680</adl:Argument>"
"<adl:Argument>abc</adl:Argument>"
"<adl:FailIfExitCodeNotEqualTo>104</adl:FailIfExitCodeNotEqualTo>"
"</adl:PreExecutable>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Application.PreExecutable.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"my-first-pre-executable", OUTJOBS.front().Application.PreExecutable.front().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                 (int)OUTJOBS.front().Application.PreExecutable.front().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"123456789",               OUTJOBS.front().Application.PreExecutable.front().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"xyz",                     OUTJOBS.front().Application.PreExecutable.front().Argument.back());
  CPPUNIT_ASSERT(                                              OUTJOBS.front().Application.PreExecutable.front().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(0,                                      OUTJOBS.front().Application.PreExecutable.front().SuccessExitCode.second);

  CPPUNIT_ASSERT_EQUAL((std::string)"foo/my-second-pre-executable", OUTJOBS.front().Application.PreExecutable.back().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                      (int)OUTJOBS.front().Application.PreExecutable.back().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"1357924680",                   OUTJOBS.front().Application.PreExecutable.back().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"abc",                          OUTJOBS.front().Application.PreExecutable.back().Argument.back());
  CPPUNIT_ASSERT(                                                   OUTJOBS.front().Application.PreExecutable.back().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(104,                                         OUTJOBS.front().Application.PreExecutable.back().SuccessExitCode.second);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Application.PreExecutable.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"my-first-pre-executable", OUTJOBS.front().Application.PreExecutable.front().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                 (int)OUTJOBS.front().Application.PreExecutable.front().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"123456789",               OUTJOBS.front().Application.PreExecutable.front().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"xyz",                     OUTJOBS.front().Application.PreExecutable.front().Argument.back());
  CPPUNIT_ASSERT(                                              OUTJOBS.front().Application.PreExecutable.front().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(0,                                      OUTJOBS.front().Application.PreExecutable.front().SuccessExitCode.second);

  CPPUNIT_ASSERT_EQUAL((std::string)"foo/my-second-pre-executable", OUTJOBS.front().Application.PreExecutable.back().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                      (int)OUTJOBS.front().Application.PreExecutable.back().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"1357924680",                   OUTJOBS.front().Application.PreExecutable.back().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"abc",                          OUTJOBS.front().Application.PreExecutable.back().Argument.back());
  CPPUNIT_ASSERT(                                                   OUTJOBS.front().Application.PreExecutable.back().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(104,                                         OUTJOBS.front().Application.PreExecutable.back().SuccessExitCode.second);
}

void ADLParserTest::PostExecutableTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:PostExecutable>"
"<adl:Path>my-first-post-executable</adl:Path>"
"<adl:Argument>987654321</adl:Argument>"
"<adl:Argument>zyx</adl:Argument>"
"<adl:FailIfExitCodeNotEqualTo>-1</adl:FailIfExitCodeNotEqualTo>"
"</adl:PostExecutable>"
"<adl:PostExecutable>"
"<adl:Path>foo/my-second-post-executable</adl:Path>"
"<adl:Argument>0864297531</adl:Argument>"
"<adl:Argument>cba</adl:Argument>"
"<adl:FailIfExitCodeNotEqualTo>401</adl:FailIfExitCodeNotEqualTo>"
"</adl:PostExecutable>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Application.PostExecutable.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"my-first-post-executable", OUTJOBS.front().Application.PostExecutable.front().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                  (int)OUTJOBS.front().Application.PostExecutable.front().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"987654321",                OUTJOBS.front().Application.PostExecutable.front().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"zyx",                      OUTJOBS.front().Application.PostExecutable.front().Argument.back());
  CPPUNIT_ASSERT(                                               OUTJOBS.front().Application.PostExecutable.front().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(-1,                                      OUTJOBS.front().Application.PostExecutable.front().SuccessExitCode.second);

  CPPUNIT_ASSERT_EQUAL((std::string)"foo/my-second-post-executable", OUTJOBS.front().Application.PostExecutable.back().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                       (int)OUTJOBS.front().Application.PostExecutable.back().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"0864297531",                    OUTJOBS.front().Application.PostExecutable.back().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"cba",                           OUTJOBS.front().Application.PostExecutable.back().Argument.back());
  CPPUNIT_ASSERT(                                                    OUTJOBS.front().Application.PostExecutable.back().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(401,                                          OUTJOBS.front().Application.PostExecutable.back().SuccessExitCode.second);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(2, (int)OUTJOBS.front().Application.PostExecutable.size());

  CPPUNIT_ASSERT_EQUAL((std::string)"my-first-post-executable", OUTJOBS.front().Application.PostExecutable.front().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                  (int)OUTJOBS.front().Application.PostExecutable.front().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"987654321",                OUTJOBS.front().Application.PostExecutable.front().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"zyx",                      OUTJOBS.front().Application.PostExecutable.front().Argument.back());
  CPPUNIT_ASSERT(                                               OUTJOBS.front().Application.PostExecutable.front().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(-1,                                      OUTJOBS.front().Application.PostExecutable.front().SuccessExitCode.second);

  CPPUNIT_ASSERT_EQUAL((std::string)"foo/my-second-post-executable", OUTJOBS.front().Application.PostExecutable.back().Path);
  CPPUNIT_ASSERT_EQUAL(2,                                       (int)OUTJOBS.front().Application.PostExecutable.back().Argument.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"0864297531",                    OUTJOBS.front().Application.PostExecutable.back().Argument.front());
  CPPUNIT_ASSERT_EQUAL((std::string)"cba",                           OUTJOBS.front().Application.PostExecutable.back().Argument.back());
  CPPUNIT_ASSERT(                                                    OUTJOBS.front().Application.PostExecutable.back().SuccessExitCode.first);
  CPPUNIT_ASSERT_EQUAL(401,                                          OUTJOBS.front().Application.PostExecutable.back().SuccessExitCode.second);
}

void ADLParserTest::LoggingDirectoryTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:LoggingDirectory>job-log</adl:LoggingDirectory>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"job-log", OUTJOBS.front().Application.LogDir);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"job-log", OUTJOBS.front().Application.LogDir);
}

void ADLParserTest::RemoteLoggingTest() {
  const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:RemoteLogging optional=\"false\">"
"<adl:ServiceType>SGAS</adl:ServiceType>"
"<adl:URL>https://sgas.eu-emi.eu/</adl:URL>"
"</adl:RemoteLogging>"
"<adl:RemoteLogging optional=\"true\">"
"<adl:ServiceType>APEL</adl:ServiceType>"
"<adl:URL>https://apel.eu-emi.eu/</adl:URL>"
"</adl:RemoteLogging>"
"<adl:RemoteLogging>"
"<adl:ServiceType>FOO</adl:ServiceType>"
"<adl:URL>https://foo.eu-emi.eu/</adl:URL>"
"</adl:RemoteLogging>"
"</adl:Application>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(3, (int)OUTJOBS.front().Application.RemoteLogging.size());
  std::list<Arc::RemoteLoggingType>::const_iterator itRLT = OUTJOBS.front().Application.RemoteLogging.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)"SGAS", itRLT->ServiceType);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://sgas.eu-emi.eu/"), itRLT->Location);
  CPPUNIT_ASSERT(!itRLT->optional);
  CPPUNIT_ASSERT(OUTJOBS.front().Application.RemoteLogging.end() != ++itRLT);
  CPPUNIT_ASSERT_EQUAL((std::string)"APEL", itRLT->ServiceType);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://apel.eu-emi.eu/"), itRLT->Location);
  CPPUNIT_ASSERT(itRLT->optional);
  CPPUNIT_ASSERT(OUTJOBS.front().Application.RemoteLogging.end() != ++itRLT);
  CPPUNIT_ASSERT_EQUAL((std::string)"FOO", itRLT->ServiceType);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://foo.eu-emi.eu/"), itRLT->Location);
  CPPUNIT_ASSERT(!itRLT->optional);
  CPPUNIT_ASSERT(OUTJOBS.front().Application.RemoteLogging.end() == ++itRLT);

  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL(3, (int)OUTJOBS.front().Application.RemoteLogging.size());
  itRLT = OUTJOBS.front().Application.RemoteLogging.begin();
  CPPUNIT_ASSERT_EQUAL((std::string)"SGAS", itRLT->ServiceType);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://sgas.eu-emi.eu/"), itRLT->Location);
  CPPUNIT_ASSERT(!itRLT->optional);
  CPPUNIT_ASSERT(OUTJOBS.front().Application.RemoteLogging.end() != ++itRLT);
  CPPUNIT_ASSERT_EQUAL((std::string)"APEL", itRLT->ServiceType);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://apel.eu-emi.eu/"), itRLT->Location);
  CPPUNIT_ASSERT(itRLT->optional);
  CPPUNIT_ASSERT(OUTJOBS.front().Application.RemoteLogging.end() != ++itRLT);
  CPPUNIT_ASSERT_EQUAL((std::string)"FOO", itRLT->ServiceType);
  CPPUNIT_ASSERT_EQUAL(Arc::URL("https://foo.eu-emi.eu/"), itRLT->Location);
  CPPUNIT_ASSERT(!itRLT->optional);
  CPPUNIT_ASSERT(OUTJOBS.front().Application.RemoteLogging.end() == ++itRLT);
}

/** Client stageable input file */
void ADLParserTest::TestInputFileClientStageable() {
  {
    const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Executable>"
"<adl:Path>my-executable</adl:Path>"
"</adl:Executable>"
"</adl:Application>"
"<adl:DataStaging>"
"<adl:InputFile>"
"<adl:Name>TestInputFileClientStageable</adl:Name>"
"</adl:InputFile>"
"<adl:InputFile>"
"<adl:Name>executable</adl:Name>"
"<adl:IsExecutable>true</adl:IsExecutable>"
"</adl:InputFile>"
"</adl:DataStaging>"
"</adl:ActivityDescription>";

    CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::InputFileType>& ifiles = OUTJOBS.front().DataStaging.InputFiles;
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestInputFileClientStageable", ifiles.front().Name);
    CPPUNIT_ASSERT(!ifiles.front().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(1, (int)ifiles.front().Sources.size());
    CPPUNIT_ASSERT_EQUAL((std::string)"executable",  ifiles.back().Name);
    CPPUNIT_ASSERT(ifiles.back().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(1, (int)ifiles.back().Sources.size());
  }

  {
    std::string tempjobdesc;
    CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "emies:adl"));
    OUTJOBS.clear();
    CPPUNIT_ASSERT(PARSER.Parse(tempjobdesc, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::InputFileType>& ifiles = OUTJOBS.front().DataStaging.InputFiles;
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestInputFileClientStageable", ifiles.front().Name);
    CPPUNIT_ASSERT(!ifiles.front().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(1, (int)ifiles.front().Sources.size());
    CPPUNIT_ASSERT_EQUAL((std::string)"executable",  ifiles.back().Name);
    CPPUNIT_ASSERT(ifiles.back().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(1, (int)ifiles.back().Sources.size());
  }
}

/** Service stageable input file */
void ADLParserTest::TestInputFileServiceStageable() {
  {
    const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Executable>"
"<adl:Path>my-executable</adl:Path>"
"</adl:Executable>"
"</adl:Application>"
"<adl:DataStaging>"
"<adl:InputFile>"
"<adl:Name>TestInputFileServiceStageable</adl:Name>"
"<adl:Source>"
"<adl:URI>https://se.eu-emi.eu/1234567890/abcdefghij/TestInputFileServiceStageable</adl:URI>"
"<adl:DelegationID>0a9b8c7d6e5f4g3h2i1j</adl:DelegationID>"
"</adl:Source>"
"<adl:Source>"
"<adl:URI>https://se-alt.eu-emi.eu/0987654321/klmnopqrst/TestInputFileServiceStageable</adl:URI>"
"<adl:DelegationID>1t2s3r4q5p6o7n8m9l0k</adl:DelegationID>"
"</adl:Source>"
"</adl:InputFile>"
"<adl:InputFile>"
"<adl:Name>executable</adl:Name>"
"<adl:Source>"
"<adl:URI>gsiftp://gsi-se.eu-emi.eu/5647382910/xyzuvwrstq/executable</adl:URI>"
"<adl:DelegationID>j1i2h3g4f5e6d7c8b9a0</adl:DelegationID>"
"</adl:Source>"
"<adl:Source>"
"<adl:URI>gsiftp://gsi-se-alt.eu-emi.eu/0192837465/qtsrwvuzyx/executable</adl:URI>"
"<adl:DelegationID>0a9b8c7d6e5f4g3h2i1j</adl:DelegationID>"
"</adl:Source>"
"<adl:IsExecutable>true</adl:IsExecutable>"
"</adl:InputFile>"
"</adl:DataStaging>"
"</adl:ActivityDescription>";

    CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::InputFileType>& ifiles = OUTJOBS.front().DataStaging.InputFiles;
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestInputFileServiceStageable", ifiles.front().Name);
    CPPUNIT_ASSERT(!ifiles.front().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.front().Sources.size());
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("https://se.eu-emi.eu/1234567890/abcdefghij/TestInputFileServiceStageable"), ifiles.front().Sources.front());
    CPPUNIT_ASSERT_EQUAL((std::string)"0a9b8c7d6e5f4g3h2i1j", ifiles.front().Sources.front().DelegationID);
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("https://se-alt.eu-emi.eu/0987654321/klmnopqrst/TestInputFileServiceStageable"), ifiles.front().Sources.back());
    CPPUNIT_ASSERT_EQUAL((std::string)"1t2s3r4q5p6o7n8m9l0k", ifiles.front().Sources.back().DelegationID);
    CPPUNIT_ASSERT_EQUAL((std::string)"executable", ifiles.back().Name);
    CPPUNIT_ASSERT(ifiles.back().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.back().Sources.size());
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("gsiftp://gsi-se.eu-emi.eu/5647382910/xyzuvwrstq/executable"), ifiles.back().Sources.front());
    CPPUNIT_ASSERT_EQUAL((std::string)"j1i2h3g4f5e6d7c8b9a0", ifiles.back().Sources.front().DelegationID);
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("gsiftp://gsi-se-alt.eu-emi.eu/0192837465/qtsrwvuzyx/executable"), ifiles.back().Sources.back());
    CPPUNIT_ASSERT_EQUAL((std::string)"0a9b8c7d6e5f4g3h2i1j", ifiles.back().Sources.back().DelegationID);
  }

  {
    std::string tempjobdesc;
    CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "emies:adl"));
    OUTJOBS.clear();
    CPPUNIT_ASSERT(PARSER.Parse(tempjobdesc, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::InputFileType>& ifiles = OUTJOBS.front().DataStaging.InputFiles;
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestInputFileServiceStageable", ifiles.front().Name);
    CPPUNIT_ASSERT(!ifiles.front().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.front().Sources.size());
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("https://se.eu-emi.eu/1234567890/abcdefghij/TestInputFileServiceStageable"), ifiles.front().Sources.front());
    CPPUNIT_ASSERT_EQUAL((std::string)"0a9b8c7d6e5f4g3h2i1j", ifiles.front().Sources.front().DelegationID);
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("https://se-alt.eu-emi.eu/0987654321/klmnopqrst/TestInputFileServiceStageable"), ifiles.front().Sources.back());
    CPPUNIT_ASSERT_EQUAL((std::string)"1t2s3r4q5p6o7n8m9l0k", ifiles.front().Sources.back().DelegationID);
    CPPUNIT_ASSERT_EQUAL((std::string)"executable", ifiles.back().Name);
    CPPUNIT_ASSERT(ifiles.back().IsExecutable);
    CPPUNIT_ASSERT_EQUAL(2, (int)ifiles.back().Sources.size());
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("gsiftp://gsi-se.eu-emi.eu/5647382910/xyzuvwrstq/executable"), ifiles.back().Sources.front());
    CPPUNIT_ASSERT_EQUAL((std::string)"j1i2h3g4f5e6d7c8b9a0", ifiles.back().Sources.front().DelegationID);
    CPPUNIT_ASSERT_EQUAL(Arc::SourceType("gsiftp://gsi-se-alt.eu-emi.eu/0192837465/qtsrwvuzyx/executable"), ifiles.back().Sources.back());
    CPPUNIT_ASSERT_EQUAL((std::string)"0a9b8c7d6e5f4g3h2i1j", ifiles.back().Sources.back().DelegationID);
  }
}

/** Client stageable output file */
void ADLParserTest::TestOutputFileClientStageable() {
  {
    const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Executable>"
"<adl:Path>my-executable</adl:Path>"
"</adl:Executable>"
"</adl:Application>"
"<adl:DataStaging>"
"<adl:OutputFile>"
"<adl:Name>TestOutputFileClientStageable</adl:Name>"
"</adl:OutputFile>"
"</adl:DataStaging>"
"</adl:ActivityDescription>";

    CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::OutputFileType>& ofiles = OUTJOBS.front().DataStaging.OutputFiles;
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestOutputFileClientStageable", ofiles.front().Name);
    CPPUNIT_ASSERT(ofiles.front().Targets.empty());
  }

  {
    std::string tempjobdesc;
    CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "emies:adl"));
    OUTJOBS.clear();
    CPPUNIT_ASSERT(PARSER.Parse(tempjobdesc, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::OutputFileType>& ofiles = OUTJOBS.front().DataStaging.OutputFiles;
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestOutputFileClientStageable", ofiles.front().Name);
    CPPUNIT_ASSERT(ofiles.front().Targets.empty());
  }
}

/** Service stageable output file */
void ADLParserTest::TestOutputFileServiceStageable() {
  {
    const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
"<adl:Application>"
"<adl:Executable>"
"<adl:Path>my-executable</adl:Path>"
"</adl:Executable>"
"</adl:Application>"
"<adl:DataStaging>"
"<adl:OutputFile>"
"<adl:Name>TestOutputFileServiceStageable</adl:Name>"
"<adl:Target>"
"<adl:URI>https://se.eu-emi.eu/1234567890/abcdefghij/TestInputFileServiceStageable</adl:URI>"
"<adl:DelegationID>0a9b8c7d6e5f4g3h2i1j</adl:DelegationID>"
"</adl:Target>"
"<adl:Target>"
"<adl:URI>https://se-alt.eu-emi.eu/0987654321/klmnopqrst/TestInputFileServiceStageable</adl:URI>"
"<adl:DelegationID>1t2s3r4q5p6o7n8m9l0k</adl:DelegationID>"
"<adl:UseIfFailure>true</adl:UseIfFailure>"
"<adl:UseIfCancel>true</adl:UseIfCancel>"
"<adl:UseIfSuccess>false</adl:UseIfSuccess>"
"</adl:Target>"
"</adl:OutputFile>"
"</adl:DataStaging>"
"</adl:ActivityDescription>";

    CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::OutputFileType>& ofiles = OUTJOBS.front().DataStaging.OutputFiles;
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestOutputFileServiceStageable", ofiles.front().Name);
    CPPUNIT_ASSERT_EQUAL(2, (int)ofiles.front().Targets.size());
    CPPUNIT_ASSERT_EQUAL(Arc::TargetType("https://se.eu-emi.eu/1234567890/abcdefghij/TestInputFileServiceStageable"), ofiles.front().Targets.front());
    CPPUNIT_ASSERT_EQUAL((std::string)"0a9b8c7d6e5f4g3h2i1j", ofiles.front().Targets.front().DelegationID);
    CPPUNIT_ASSERT(!ofiles.front().Targets.front().UseIfFailure);
    CPPUNIT_ASSERT(!ofiles.front().Targets.front().UseIfCancel);
    CPPUNIT_ASSERT(ofiles.front().Targets.front().UseIfSuccess);
    CPPUNIT_ASSERT_EQUAL(Arc::TargetType("https://se-alt.eu-emi.eu/0987654321/klmnopqrst/TestInputFileServiceStageable"), ofiles.front().Targets.back());
    CPPUNIT_ASSERT_EQUAL((std::string)"1t2s3r4q5p6o7n8m9l0k", ofiles.front().Targets.back().DelegationID);
    CPPUNIT_ASSERT(ofiles.front().Targets.back().UseIfFailure);
    CPPUNIT_ASSERT(ofiles.front().Targets.back().UseIfCancel);
    CPPUNIT_ASSERT(!ofiles.front().Targets.back().UseIfSuccess);
  }

  {
    std::string tempjobdesc;
    CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "emies:adl"));
    OUTJOBS.clear();
    CPPUNIT_ASSERT(PARSER.Parse(tempjobdesc, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::OutputFileType>& ofiles = OUTJOBS.front().DataStaging.OutputFiles;
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestOutputFileServiceStageable", ofiles.front().Name);
    CPPUNIT_ASSERT_EQUAL(2, (int)ofiles.front().Targets.size());
    CPPUNIT_ASSERT_EQUAL(Arc::TargetType("https://se.eu-emi.eu/1234567890/abcdefghij/TestInputFileServiceStageable"), ofiles.front().Targets.front());
    CPPUNIT_ASSERT_EQUAL((std::string)"0a9b8c7d6e5f4g3h2i1j", ofiles.front().Targets.front().DelegationID);
    CPPUNIT_ASSERT(!ofiles.front().Targets.front().UseIfFailure);
    CPPUNIT_ASSERT(!ofiles.front().Targets.front().UseIfCancel);
    CPPUNIT_ASSERT(ofiles.front().Targets.front().UseIfSuccess);
    CPPUNIT_ASSERT_EQUAL(Arc::TargetType("https://se-alt.eu-emi.eu/0987654321/klmnopqrst/TestInputFileServiceStageable"), ofiles.front().Targets.back());
    CPPUNIT_ASSERT_EQUAL((std::string)"1t2s3r4q5p6o7n8m9l0k", ofiles.front().Targets.back().DelegationID);
    CPPUNIT_ASSERT(ofiles.front().Targets.back().UseIfFailure);
    CPPUNIT_ASSERT(ofiles.front().Targets.back().UseIfCancel);
    CPPUNIT_ASSERT(!ofiles.front().Targets.back().UseIfSuccess);
  }
}

/** Service stageable output file with locations and options */
void ADLParserTest::TestOutputFileLocationsServiceStageable() {
  {
    const std::string adl = "<?xml version=\"1.0\"?>"
"<adl:ActivityDescription xmlns:adl=\"http://www.eu-emi.eu/es/2010/12/adl\" xmlns:nordugrid-adl=\"http://www.nordugrid.org/es/2011/12/nordugrid-adl\">"
  "<adl:Application>"
    "<adl:Executable>"
      "<adl:Path>my-executable</adl:Path>"
    "</adl:Executable>"
  "</adl:Application>"
  "<adl:DataStaging>"
    "<adl:OutputFile>"
      "<adl:Name>TestOutputFileServiceStageable</adl:Name>"
      "<adl:Target>"
        "<adl:URI>lfc://lfc.eu-emi.eu:5010/1234567890/abcdefghij/TestInputFileServiceStageable</adl:URI>"
        "<adl:Option>"
          "<adl:Name>location</adl:Name>"
          "<adl:Value>https://se.eu-emi.eu:443/0987654321/klmnopqrst/TestInputFileServiceStageable</adl:Value>"
        "</adl:Option>"
        "<adl:Option>"
          "<adl:Name>overwrite</adl:Name>"
          "<adl:Value>yes</adl:Value>"
        "</adl:Option>"
      "</adl:Target>"
    "</adl:OutputFile>"
  "</adl:DataStaging>"
"</adl:ActivityDescription>";

    CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::OutputFileType>& ofiles = OUTJOBS.front().DataStaging.OutputFiles;
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestOutputFileServiceStageable", ofiles.front().Name);
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.front().Targets.size());
    CPPUNIT_ASSERT_EQUAL((std::string)"lfc://lfc.eu-emi.eu:5010/1234567890/abcdefghij/TestInputFileServiceStageable", ofiles.front().Targets.front().str());
    CPPUNIT_ASSERT_EQUAL((std::string)"yes", ofiles.front().Targets.front().Option("overwrite"));
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.front().Targets.front().Locations().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"https://se.eu-emi.eu:443/0987654321/klmnopqrst/TestInputFileServiceStageable", ofiles.front().Targets.front().Locations().front().str());
  }

  {
    std::string tempjobdesc;
    CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), tempjobdesc, "emies:adl"));
    OUTJOBS.clear();
    CPPUNIT_ASSERT(PARSER.Parse(tempjobdesc, OUTJOBS));
    CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());

    const std::list<Arc::OutputFileType>& ofiles = OUTJOBS.front().DataStaging.OutputFiles;
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.size());

    CPPUNIT_ASSERT_EQUAL((std::string)"TestOutputFileServiceStageable", ofiles.front().Name);
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.front().Targets.size());
    CPPUNIT_ASSERT_EQUAL((std::string)"lfc://lfc.eu-emi.eu:5010/1234567890/abcdefghij/TestInputFileServiceStageable", ofiles.front().Targets.front().str());
    CPPUNIT_ASSERT_EQUAL((std::string)"yes", ofiles.front().Targets.front().Option("overwrite"));
    CPPUNIT_ASSERT_EQUAL(1, (int)ofiles.front().Targets.front().Locations().size());
    CPPUNIT_ASSERT_EQUAL((std::string)"https://se.eu-emi.eu:443/0987654321/klmnopqrst/TestInputFileServiceStageable", ofiles.front().Targets.front().Locations().front().str());
  }
}



CPPUNIT_TEST_SUITE_REGISTRATION(ADLParserTest);
