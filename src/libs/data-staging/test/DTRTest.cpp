#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include "../DTR.h"

using namespace DataStaging;

class DTRTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(DTRTest);
  CPPUNIT_TEST(TestDTRConstructor);
  CPPUNIT_TEST(TestDTREndpoints);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestDTRConstructor();
  void TestDTREndpoints();

  void setUp();
  void tearDown();

private:
  std::list<DTRLogDestination> logs;
  char const * log_name;
  Arc::UserConfig cfg;
};

void DTRTest::setUp() {
  logs.clear();
  const std::list<Arc::LogDestination*>& destinations = Arc::Logger::getRootLogger().getDestinations();
  for(std::list<Arc::LogDestination*>::const_iterator dest = destinations.begin(); dest != destinations.end(); ++dest) {
    logs.push_back(*dest);
  }
  log_name = "DataStagingTest";
}

void DTRTest::tearDown() {
}

void DTRTest::TestDTRConstructor() {
  std::string jobid("123456789");
  std::string source("mock://mocksrc/1");
  std::string destination("mock://mockdest/1");
  DataStaging::DTR_ptr dtr(new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name));
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(!dtr->get_id().empty());

  // Copy constructor
  DataStaging::DTR_ptr dtr2(dtr);
  CPPUNIT_ASSERT(*dtr2);
  CPPUNIT_ASSERT_EQUAL(dtr->get_id(), dtr2->get_id());
  // a new DataHandle object is created for the new DTR so they should
  // not be equal. Why does this test pass????
  CPPUNIT_ASSERT_EQUAL(dtr->get_source(), dtr2->get_source());
  CPPUNIT_ASSERT_EQUAL(dtr->get_owner(), dtr2->get_owner());
  CPPUNIT_ASSERT_EQUAL(dtr->get_status().GetStatus(), dtr2->get_status().GetStatus());

  // check that creating and destroying a copy doesn't affect the original
  {
    DataStaging::DTR_ptr dtr3(dtr);
    CPPUNIT_ASSERT(*dtr3);
  }
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mocksrc/1"), dtr->get_source()->str());

  // make a bad DTR
  source = "myprocotol://blabla/file1";
  DataStaging::DTR_ptr dtr4(new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name));
  CPPUNIT_ASSERT(!(*dtr4));

  // bad DTR copying to itself
  DataStaging::DTR_ptr dtr5(new DataStaging::DTR(source, source, cfg, jobid, Arc::User().get_uid(), logs, log_name));
  CPPUNIT_ASSERT(!(*dtr5));
}

void DTRTest::TestDTREndpoints() {
  std::string jobid("123456789");
  std::string source("mock://mocksrc/1");
  std::string destination("mock://mockdest/1");
  DataStaging::DTR_ptr dtr(new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name));
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mocksrc/1"), dtr->get_source()->str());
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mockdest/1"), dtr->get_destination()->str());

  // create a bad url
  source = "mock:/file1";
  DataStaging::DTR_ptr dtrbad(new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name));
  CPPUNIT_ASSERT(!dtrbad->get_source()->GetURL());

  // TODO DTR validity
}

CPPUNIT_TEST_SUITE_REGISTRATION(DTRTest);
