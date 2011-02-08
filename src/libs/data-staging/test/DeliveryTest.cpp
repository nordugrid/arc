#include <cppunit/extensions/HelperMacros.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <arc/FileUtils.h>
#include <arc/GUID.h>
#include <arc/credential/Credential.h>
#include <arc/UserConfig.h>

#include "../DTRStatus.h"
#include "../DTR.h"
#include "../DataDelivery.h"

#define CONNECTION_TIMEOUT 20

class DeliveryTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(DeliveryTest);
  CPPUNIT_TEST(TestDeliverySimple);
  CPPUNIT_TEST(TestDeliveryFailure);
  CPPUNIT_TEST(TestDeliveryUnsupported);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestDeliverySimple();
  void TestDeliveryFailure();
  void TestDeliveryUnsupported();
  void setUp();
  void tearDown();

private:
  std::string dest_file;
  Arc::Logger * logger;
};

void DeliveryTest::setUp() {
  remove(dest_file.c_str());
  logger = new Arc::Logger(Arc::Logger::getRootLogger(), "DataStagingTest");
}

void DeliveryTest::tearDown() {
  remove(dest_file.c_str());
  delete logger;
  logger = NULL;
}

void DeliveryTest::TestDeliverySimple() {

  // Fake environment
  Arc::UserConfig cfg;
  std::string jobid("1234");

  // Remote source, local destination
  std::string source("http://www.nordugrid.org");
  dest_file = "/tmp/file1";
  std::string destination("file:" + dest_file);
  DataStaging::DTR dtr(source,destination,cfg,jobid,getuid(),logger);

  // Pass DTR to Delivery
  DataStaging::DataDelivery delivery;
  delivery.start();
  delivery.receiveDTR(dtr);
  DataStaging::DTRStatus status = dtr.get_status();

  // Wait for result. It must be either ERROR or TRANSFERRED at end.
  // During transfer state may be NULL or TRANSFERRING
  for(int cnt=0;;++cnt) {
    status = dtr.get_status();
    if(status == DataStaging::DTRStatus::ERROR) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRED) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRING) {
    } else if(status == DataStaging::DTRStatus::NULL_STATE) {
    } else {
      break;
    }
    CPPUNIT_ASSERT(cnt < 100);
    Glib::usleep(100000);
  }
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::TRANSFERRED, status.GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NO_ERROR, dtr.get_error_status().GetErrorStatus());
}

void DeliveryTest::TestDeliveryFailure() {

  // Fake environment
  Arc::UserConfig cfg;
  std::string jobid("1234");

  // Remote source, local destination
  std::string source("http://www.nordugrid.org/no_such_file.html");
  dest_file = "/tmp/file2";
  std::string destination("file:" + dest_file);
  DataStaging::DTR dtr(source,destination,cfg,jobid,getuid(),logger);

  // Pass DTR to Delivery
  DataStaging::DataDelivery delivery;
  delivery.start();
  delivery.receiveDTR(dtr);
  DataStaging::DTRStatus status = dtr.get_status();

  // Wait for result. It must be either ERROR or TRANSFERRED at end.
  // During transfer state may be NULL or TRANSFERRING
  for(int cnt=0;;++cnt) {
    status = dtr.get_status();
    if(status == DataStaging::DTRStatus::ERROR) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRED) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRING) {
    } else if(status == DataStaging::DTRStatus::NULL_STATE) {
    } else {
      break;
    }
    CPPUNIT_ASSERT(cnt < 100);
    Glib::usleep(100000);
  }
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::TRANSFERRED, status.GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR, dtr.get_error_status().GetErrorStatus());
}

void DeliveryTest::TestDeliveryUnsupported() {

  // Fake environment
  Arc::UserConfig cfg;
  std::string jobid("1234");

  // Remote source, local destination
  std::string source("proto://host/file");
  dest_file = "/tmp/file2";
  std::string destination("file:" + dest_file);
  DataStaging::DTR dtr(source,destination,cfg,jobid,getuid(),logger);

  // Pass DTR to Delivery
  DataStaging::DataDelivery delivery;
  delivery.start();
  delivery.receiveDTR(dtr);
  DataStaging::DTRStatus status = dtr.get_status();

  // Wait for result. It must be either ERROR or TRANSFERRED at end.
  // During transfer state may be NULL or TRANSFERRING
  for(int cnt=0;;++cnt) {
    status = dtr.get_status();
    if(status == DataStaging::DTRStatus::ERROR) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRED) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRING) {
    } else if(status == DataStaging::DTRStatus::NULL_STATE) {
    } else {
      break;
    }
    CPPUNIT_ASSERT(cnt < 100);
    Glib::usleep(100000);
  }
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::ERROR, status.GetStatus());
}

CPPUNIT_TEST_SUITE_REGISTRATION(DeliveryTest);
