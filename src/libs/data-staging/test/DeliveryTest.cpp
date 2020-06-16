#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <arc/ArcLocation.h>
#include <arc/FileUtils.h>
#include <arc/UserConfig.h>

#include "../DTRStatus.h"
#include "../DTR.h"
#include "../DataDelivery.h"

using namespace DataStaging;

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
  std::list<DTRLogDestination> logs;
  char const * log_name;
  Arc::UserConfig cfg;
};

void DeliveryTest::setUp() {
  // Hack to make sure DataStagingDelivery executable in the parent dir is used
  // A fake ARC location is used and a symlink is created in the libexec subdir
  // to the DataStagingDelivery in the parent dir. TODO: maybe put a test flag
  // in DTR code which tells it to use this local executable.
  Arc::DirCreate(std::string("../tmp/")+std::string(PKGLIBSUBDIR), S_IRWXU, true);
  Arc::ArcLocation::Init("../tmp/x/x");
  Arc::FileLink("../../../DataStagingDelivery", std::string("../tmp/")+std::string(PKGLIBSUBDIR)+std::string("/DataStagingDelivery"), true);
  logs.clear();
  const std::list<Arc::LogDestination*>& destinations = Arc::Logger::getRootLogger().getDestinations();
  for(std::list<Arc::LogDestination*>::const_iterator dest = destinations.begin(); dest != destinations.end(); ++dest) {
    logs.push_back(*dest);
  }

  log_name = "DataStagingTest";
}

void DeliveryTest::tearDown() {
  Arc::DirDelete("../tmp");
}

void DeliveryTest::TestDeliverySimple() {

  std::string source("mock://mocksrc/1");
  std::string destination("mock://mockdest/1");
  std::string jobid("1234");
  DataStaging::DTR_ptr dtr(new DataStaging::DTR(source,destination,cfg,jobid,Arc::User().get_uid(),logs,log_name));
  CPPUNIT_ASSERT(*dtr);

  // Pass DTR to Delivery
  DataStaging::DataDelivery delivery;
  delivery.start();
  delivery.receiveDTR(dtr);
  DataStaging::DTRStatus status = dtr->get_status();

  // Wait for result. It must be either ERROR or TRANSFERRED at end.
  // During transfer state may be NULL or TRANSFERRING
  for(int cnt=0;;++cnt) {
    status = dtr->get_status();
    if(status == DataStaging::DTRStatus::ERROR) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRED) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRING) {
    } else if(status == DataStaging::DTRStatus::NULL_STATE) {
    } else {
      break;
    }
    CPPUNIT_ASSERT(cnt < 300); // 30s limit on transfer time
    Glib::usleep(100000);
  }
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::TRANSFERRED, status.GetStatus());
  CPPUNIT_ASSERT_EQUAL_MESSAGE(dtr->get_error_status().GetDesc(), DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
}

void DeliveryTest::TestDeliveryFailure() {

  std::string source("fail://mocksrc/1");
  std::string destination("fail://mockdest/1");
  std::string jobid("1234");
  DataStaging::DTR_ptr dtr(new DataStaging::DTR(source,destination,cfg,jobid,Arc::User().get_uid(),logs,log_name));
  CPPUNIT_ASSERT(*dtr);

  // Pass DTR to Delivery
  DataStaging::DataDelivery delivery;
  delivery.start();
  delivery.receiveDTR(dtr);
  DataStaging::DTRStatus status = dtr->get_status();

  // Wait for result. It must be either ERROR or TRANSFERRED at end.
  // During transfer state may be NULL or TRANSFERRING
  for(int cnt=0;;++cnt) {
    status = dtr->get_status();
    if(status == DataStaging::DTRStatus::ERROR) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRED) {
      break;
    } else if(status == DataStaging::DTRStatus::TRANSFERRING) {
    } else if(status == DataStaging::DTRStatus::NULL_STATE) {
    } else {
      break;
    }
    CPPUNIT_ASSERT(cnt < 200); // 20s limit on transfer time
    Glib::usleep(100000);
  }
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::TRANSFERRED, status.GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::TEMPORARY_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
}

void DeliveryTest::TestDeliveryUnsupported() {

  std::string source("proto://host/file");
  std::string destination("mock://mockdest/1");
  std::string jobid("1234");
  DataStaging::DTR_ptr dtr(new DataStaging::DTR(source,destination,cfg,jobid,Arc::User().get_uid(),logs,log_name));
  CPPUNIT_ASSERT(!(*dtr));

  // Pass DTR to Delivery
  DataStaging::DataDelivery delivery;
  delivery.start();
  delivery.receiveDTR(dtr);

  // DTR should be checked by delivery and immediately set to TRANSFERRED
  // with error status set to LOGIC error
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::TRANSFERRED, dtr->get_status().GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::INTERNAL_LOGIC_ERROR, dtr->get_error_status().GetErrorStatus());
}

CPPUNIT_TEST_SUITE_REGISTRATION(DeliveryTest);
