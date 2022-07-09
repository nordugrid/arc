#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <arc/GUID.h>
#include <arc/FileUtils.h>

#include "../DTRStatus.h"
#include "../Processor.h"

using namespace DataStaging;

class ProcessorTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(ProcessorTest);
  CPPUNIT_TEST(TestPreClean);
  CPPUNIT_TEST(TestCacheCheck);
  CPPUNIT_TEST(TestResolve);
  CPPUNIT_TEST(TestQueryReplica);
  CPPUNIT_TEST(TestReplicaRegister);
  CPPUNIT_TEST(TestCacheProcess);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestPreClean();
  void TestCacheCheck();
  void TestResolve();
  void TestQueryReplica();
  void TestReplicaRegister();
  void TestCacheProcess();
  void setUp();
  void tearDown();

private:
  std::list<DTRLogDestination> logs;
  char const * log_name;
  Arc::UserConfig cfg;
  std::string tmpdir;
};

void ProcessorTest::setUp() {
  logs.clear();
  const std::list<Arc::LogDestination*>& destinations = Arc::Logger::getRootLogger().getDestinations();
  for(std::list<Arc::LogDestination*>::const_iterator dest = destinations.begin(); dest != destinations.end(); ++dest) {
    logs.push_back(*dest);
  }
  log_name = "DataStagingTest";
}

void ProcessorTest::tearDown() {
  if (!tmpdir.empty()) Arc::DirDelete(tmpdir);
}

void ProcessorTest::TestPreClean() {

  // Note: mock doesn't really delete, but reports success
  std::string jobid("123456789");
  std::string source("mock://mocksrc/1");
  std::string destination("mock://mockdest;overwrite=yes/1");

  DataStaging::DTR_ptr dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::PRE_CLEAN);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);

  DataStaging::Processor processor;
  processor.start();
  processor.receiveDTR(dtr);
  // sleep while thread deletes
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::PRE_CLEANED) Glib::usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::PRE_CLEANED, dtr->get_status().GetStatus());

  // use a non-existent file
  destination = "fail://badhost;overwrite=yes/file1";
  dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::PRE_CLEAN);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread deletes
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::PRE_CLEANED) Glib::usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::TEMPORARY_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
  // PRE_CLEANED is the correct status even after an error
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::PRE_CLEANED, dtr->get_status().GetStatus());
}

void ProcessorTest::TestCacheCheck() {

  // create tmp cache dir for test
  CPPUNIT_ASSERT(Arc::TmpDirCreate(tmpdir));

  std::string session(tmpdir);
  session += "/session";
  std::string cache_dir(tmpdir);
  cache_dir += "/cache";
  DataStaging::DTRCacheParameters cache_param;
  cache_param.cache_dirs.push_back(cache_dir);

  // use non-cacheable input and check it cannot be not cached
  std::string jobid("123456789");
  std::string source("mock://mocksrc;cache=no/1");
  std::string destination(std::string(session+"/file1"));

  DataStaging::DTR_ptr dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::NON_CACHEABLE);
  dtr->set_cache_parameters(cache_param);

  // use cacheable input - set invariant since mock does not set a modification
  // time and so cache file will appear outdated
  source = "mock://mocksrc;cache=invariant/1";

  dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::CACHEABLE);
  dtr->set_cache_parameters(cache_param);
  dtr->set_status(DataStaging::DTRStatus::CHECK_CACHE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  std::string cache_file(cache_dir + "/data/58/32ec5285b5990e13fd6628af93ea2b751dac7b");

  DataStaging::Processor processor;
  processor.start();
  processor.receiveDTR(dtr);

  // sleep while thread checks cache
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_CHECKED) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_CHECKED, dtr->get_status().GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::CACHEABLE, dtr->get_cache_state());
  CPPUNIT_ASSERT_EQUAL(cache_file, dtr->get_cache_file());

  // locked file
  std::string lock_file(cache_file + ".lock");
  int fd = ::open(lock_file.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
  CPPUNIT_ASSERT(fd);
  char lock_contents[] = "1@localhost";
  CPPUNIT_ASSERT(write(fd, lock_contents, sizeof(lock_contents)) > 0);
  CPPUNIT_ASSERT_EQUAL(0, close(fd));

  dtr->set_status(DataStaging::DTRStatus::CHECK_CACHE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread checks cache
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_WAIT) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_WAIT, dtr->get_status().GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::CACHE_LOCKED, dtr->get_cache_state());
  CPPUNIT_ASSERT_EQUAL(0, remove(lock_file.c_str()));

  // write cache file
  fd = ::open(cache_file.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
  CPPUNIT_ASSERT(fd);
  char cache_file_contents[] = "abcde";
  CPPUNIT_ASSERT(write(fd, cache_file_contents, sizeof(cache_file_contents)) > 0);
  CPPUNIT_ASSERT_EQUAL(0, close(fd));

  // check again, should return already present
  dtr->set_status(DataStaging::DTRStatus::CHECK_CACHE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread checks cache
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_CHECKED) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_CHECKED, dtr->get_status().GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::CACHE_ALREADY_PRESENT, dtr->get_cache_state());
  CPPUNIT_ASSERT_EQUAL(0, remove(cache_file.c_str()));

  // test files using guids are handled properly
  source = "mock://mocksrc/1:guid=4a2b61aa-1e57-4d32-9f23-873a9c9b9aed";

  dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::CACHEABLE);
  dtr->set_cache_parameters(cache_param);
  dtr->set_status(DataStaging::DTRStatus::CHECK_CACHE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  cache_file = cache_dir + "/data/ba/bb0555ddfccde73069558aacfe512ea42c8c79";

  processor.receiveDTR(dtr);

  // sleep while thread checks cache
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_CHECKED) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_CHECKED, dtr->get_status().GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::CACHEABLE, dtr->get_cache_state());
  CPPUNIT_ASSERT_EQUAL(cache_file, dtr->get_cache_file());
}

void ProcessorTest::TestResolve() {

  // Note: using mock in resolve doesn't really test resolving since mock is
  // not a DataPointIndex
  DataStaging::Processor processor;
  processor.start();

  std::string jobid("123456789");

  // resolve a good source
  std::string source("mock://mocksrc/1");
  std::string destination("mock://mockdest/1");
  DataStaging::DTR_ptr dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::RESOLVE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread resolves
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED)
    Glib::usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

  // check that it found replicas
  CPPUNIT_ASSERT(dtr->get_source()->HaveLocations());

  /* This part can be uncommented if a mock index DataPoint exists
  // pre-register a good destination
  source = "mock://mocksrc/1";
  destination = "mockindex://mock://mockdest/1@mockindexdest/1";

  dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::RESOLVE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread resolves
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) Glib::usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

  // check that it added the destination replica
  CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mockdest/1"), dtr->get_destination()->CurrentLocation().str());

  std::list<Arc::FileInfo> files;
  CPPUNIT_ASSERT(dtr->get_destination()->List(files));
  CPPUNIT_ASSERT_EQUAL(1, (int)files.size());
  CPPUNIT_ASSERT_EQUAL(std::string("mockindex://mockindexdest/1"), files.front().GetName());

  // test replication
  source = "mockindex://mockdestindex/ABCDE";
  destination = "mockindex://mock://mockdest/ABCDE@mockindexdest/ABCDE";
  dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::RESOLVE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  dtr->set_replication(true); // usually set automatically by scheduler
  processor.receiveDTR(dtr);

  // sleep while thread resolves
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) Glib::usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

  // check that it found replicas
  CPPUNIT_ASSERT(dtr->get_source()->HaveLocations());

  // check that it added the destination replica
  CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mockdest/ABCDE"), dtr->get_destination()->CurrentLocation().str());

  // copy to an existing LFN from a different LFN
  source = "mock://mocksrc/2";
  destination = "mockindex://mock://mockdest/2@mockindexdest/ABCDE";

  dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::RESOLVE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread resolves
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) Glib::usleep(100);
  // will fail since force_registration is not set
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

  // set force registration and try again
  dtr->set_force_registration(true);
  dtr->reset_error_status();
  dtr->set_status(DataStaging::DTRStatus::RESOLVE);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread resolves
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) Glib::usleep(100);
  // should be successful now
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

  // check that it added the destination replica
  CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mockdest/2"), dtr->get_destination()->CurrentLocation().str());
  */
}

void ProcessorTest::TestQueryReplica() {

  // query a valid file
  std::string jobid("123456789");
  std::string source("mock://mocksrc/1");
  std::string destination("mock://mockdest/1");

  DataStaging::DTR_ptr dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);

  DataStaging::Processor processor;
  processor.start();
  dtr->set_status(DataStaging::DTRStatus::QUERY_REPLICA);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while replica is queried
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::REPLICA_QUERIED) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::REPLICA_QUERIED, dtr->get_status().GetStatus());

  // invalid file
  source = "fail://mocksrc/1";
  dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);

  dtr->set_status(DataStaging::DTRStatus::QUERY_REPLICA);
  DataStaging::DTR::push(dtr, DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while replica is queried
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::REPLICA_QUERIED) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::TEMPORARY_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::REPLICA_QUERIED, dtr->get_status().GetStatus());

}

void ProcessorTest::TestReplicaRegister() {

  /* Needs mock index DMC
  DataStaging::Processor processor;
  processor.start();

  std::string jobid("123456789");

  // register a file
  std::string source("mock://mocksrc/1");
  std::string destination("mockindex://mock://mockdest/1@mockindexdest/1");

  DataStaging::DTR_ptr dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);

  // have to resolve first
  CPPUNIT_ASSERT(dtr->get_destination()->Resolve(false).Passed());
  CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mockdest/1"), dtr->get_destination()->CurrentLocation().str());

  // pre-register
  CPPUNIT_ASSERT(dtr->get_destination()->PreRegister(false, false).Passed());

  // post-register
  dtr->set_status(DataStaging::DTRStatus::REGISTER_REPLICA);
  DataStaging::DTR::push(dtr, DataStaging::POST_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while thread resgisters
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::REPLICA_REGISTERED) Glib::usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::REPLICA_REGISTERED, dtr->get_status().GetStatus());

  // check registration is ok
  Arc::FileInfo file;
  Arc::DataPoint::DataPointInfoType verb = (Arc::DataPoint::DataPointInfoType)(Arc::DataPoint::INFO_TYPE_CONTENT | Arc::DataPoint::INFO_TYPE_STRUCT);
  CPPUNIT_ASSERT(dtr->get_destination()->Stat(file, verb).Passed());
  std::list<Arc::URL> replicas = file.GetURLs();
  CPPUNIT_ASSERT_EQUAL(1, (int)replicas.size());
  CPPUNIT_ASSERT_EQUAL(std::string("mock://mockdest/1"), replicas.front().str());

  // clean up
  CPPUNIT_ASSERT(dtr->get_destination()->Unregister(true).Passed());
  */
}

void ProcessorTest::TestCacheProcess() {

  CPPUNIT_ASSERT(Arc::TmpDirCreate(tmpdir));

  std::string session(tmpdir);
  session += "/session";
  std::string cache_dir(tmpdir);
  cache_dir += "/cache";
  DataStaging::DTRCacheParameters cache_param;
  cache_param.cache_dirs.push_back(cache_dir);

  std::string jobid("123456789");
  std::string source("mock://mocksrc/1");
  std::string destination(std::string(session+"/file1"));

  DataStaging::DTR_ptr dtr = new DataStaging::DTR(source, destination, cfg, jobid, Arc::User().get_uid(), logs, log_name);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::CACHEABLE);
  dtr->set_cache_parameters(cache_param);

  // process with no cache file present
  std::string cache_file(cache_dir + "/data/58/32ec5285b5990e13fd6628af93ea2b751dac7b");
  remove(cache_file.c_str());

  DataStaging::Processor processor;
  processor.start();
  dtr->set_status(DataStaging::DTRStatus::PROCESS_CACHE);
  DataStaging::DTR::push(dtr, DataStaging::POST_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while cache is processed
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_PROCESSED) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::CACHE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_PROCESSED, dtr->get_status().GetStatus());

  // create cache file and try again
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(cache_dir+"/data/58"), 0700, true));
  int fd = ::open(cache_file.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
  CPPUNIT_ASSERT(fd);
  char cache_file_contents[] = "abcde";
  CPPUNIT_ASSERT_EQUAL_MESSAGE(Arc::StrError(errno), (int)sizeof(cache_file_contents), (int)write(fd, cache_file_contents, sizeof(cache_file_contents)));
  CPPUNIT_ASSERT_EQUAL(0, close(fd));

  dtr->reset_error_status();
  dtr->set_status(DataStaging::DTRStatus::PROCESS_CACHE);
  DataStaging::DTR::push(dtr, DataStaging::POST_PROCESSOR);
  processor.receiveDTR(dtr);

  // sleep while cache is processed
  while (dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_PROCESSED) Glib::usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_PROCESSED, dtr->get_status().GetStatus());

  // check correct links exist
  struct stat st;
  CPPUNIT_ASSERT_EQUAL(0, stat(std::string(cache_dir + "/joblinks/123456789/file1").c_str(), &st));
  CPPUNIT_ASSERT_EQUAL(0, stat(std::string(session + "/file1").c_str(), &st));
}

CPPUNIT_TEST_SUITE_REGISTRATION(ProcessorTest);
