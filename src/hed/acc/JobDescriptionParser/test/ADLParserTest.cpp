#include <string>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/client/JobDescription.h>

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
  CPPUNIT_TEST(PreExecutableTest);
  CPPUNIT_TEST(PostExecutableTest);
  CPPUNIT_TEST_SUITE_END();

public:
  ADLParserTest() {}

  void setUp();
  void tearDown();
  void JobNameTest();
  void DescriptionTest();
  void TypeTest();
  void AnnotationTest();
  void ActivityOldIDTest();
  void ExecutableTest();
  void PreExecutableTest();
  void PostExecutableTest();

private:
  Arc::JobDescription INJOB;
  std::list<Arc::JobDescription> OUTJOBS;
  Arc::ADLParser PARSER;

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

CPPUNIT_TEST_SUITE_REGISTRATION(ADLParserTest);
