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
"<adl:Name>EMI-ADL-full</adl:Name>"
"</adl:ActivityIdentification>"
"</adl:ActivityDescription>";

  CPPUNIT_ASSERT(PARSER.Parse(adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"EMI-ADL-full", OUTJOBS.front().Identification.JobName);
  
  std::string parsed_adl;
  CPPUNIT_ASSERT(PARSER.UnParse(OUTJOBS.front(), parsed_adl, "emies:adl"));
  OUTJOBS.clear();
  CPPUNIT_ASSERT(PARSER.Parse(parsed_adl, OUTJOBS));

  CPPUNIT_ASSERT_EQUAL(1, (int)OUTJOBS.size());
  CPPUNIT_ASSERT_EQUAL((std::string)"EMI-ADL-full", OUTJOBS.front().Identification.JobName);
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

CPPUNIT_TEST_SUITE_REGISTRATION(ADLParserTest);
