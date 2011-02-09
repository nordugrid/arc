#include <cppunit/extensions/HelperMacros.h>

#include "../DTR.h"

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
  Arc::Logger * logger;
};

void DTRTest::setUp() {
  logger = new Arc::Logger(Arc::Logger::getRootLogger(), "DataStagingTest");
}

void DTRTest::tearDown() {
  delete logger;
  logger = NULL;
}

void DTRTest::TestDTRConstructor() {
  std::string jobid("123456789");
  std::string source("http://localhost/file1");
  std::string destination("/tmp/file1");
  Arc::UserConfig cfg;
  DataStaging::DTR dtr(source, destination, cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(!dtr.get_id().empty());

  // Copy constructor
  DataStaging::DTR dtr2(dtr);
  CPPUNIT_ASSERT(dtr2);
  CPPUNIT_ASSERT_EQUAL(dtr.get_id(), dtr2.get_id());
  // a new DataHandle object is created for the new DTR so they should
  // not be equal. Why does this test pass????
  CPPUNIT_ASSERT_EQUAL(dtr.get_source(), dtr2.get_source());
  CPPUNIT_ASSERT_EQUAL(dtr.get_owner(), dtr2.get_owner());
  CPPUNIT_ASSERT_EQUAL(dtr.get_status().GetStatus(), dtr2.get_status().GetStatus());

  // check that creating and destroying a copy doesn't affect the original
  {
    DataStaging::DTR dtr3(dtr);
    CPPUNIT_ASSERT(dtr3);
  }
  CPPUNIT_ASSERT_EQUAL(std::string("http://localhost:80/file1"), dtr.get_source()->str());

  // make a bad DTR
  source = "myprocotol://blabla/file1";
  DataStaging::DTR dtr4(source, destination, cfg, jobid, getuid(), logger);
  CPPUNIT_ASSERT(!dtr4);
}

void DTRTest::TestDTREndpoints() {
  std::string jobid("123456789");
  std::string source("http://localhost/file1");
  std::string destination("/tmp/file1");
  Arc::UserConfig cfg;
  DataStaging::DTR dtr(source, destination, cfg, jobid, getuid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT_EQUAL(std::string("http://localhost:80/file1"), dtr.get_source()->str());
  CPPUNIT_ASSERT_EQUAL(std::string("file:/tmp/file1"), dtr.get_destination()->str());

  // create a bad url
  source = "http:/file1";
  DataStaging::DTR dtrbad(source, destination, cfg, jobid, getuid(), logger);
  CPPUNIT_ASSERT(!dtrbad.get_source()->GetURL());

  // TODO DTR validity
}

CPPUNIT_TEST_SUITE_REGISTRATION(DTRTest);
