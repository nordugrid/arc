#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <arc/FileUtils.h>
#include <arc/GUID.h>
#include <arc/credential/Credential.h>

#include "../DTRStatus.h"
#include "../Processor.h"

#define CONNECTION_TIMEOUT 20

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
  Arc::Logger * logger;
  Arc::UserConfig * cfg;
  bool valid_proxy;
};

void ProcessorTest::setUp() {
  logger = new Arc::Logger(Arc::Logger::getRootLogger(), "DataStagingTest");
  // a valid proxy is needed for some tests - check whether one exists
  valid_proxy = false;
  cfg = new Arc::UserConfig();
  Arc::Credential cred(cfg->ProxyPath(), cfg->ProxyPath(), cfg->CACertificatesDirectory(), cfg->CACertificatePath());
  if (cred.IsValid())
    valid_proxy = true;
}

void ProcessorTest::tearDown() {
  delete cfg;
  cfg = NULL;
  delete logger;
  logger = NULL;
}

void ProcessorTest::TestPreClean() {

  std::string jobid("123456789");
  std::string source("http://localhost/file1");
  std::string destination("/tmp/file1");

  // create destination file
  int h = ::open(destination.c_str(), O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR);
  CPPUNIT_ASSERT(h > 0);
  CPPUNIT_ASSERT_EQUAL(0, close(h));
  struct stat st;
  CPPUNIT_ASSERT(Arc::FileStat(destination, &st, true));
  CPPUNIT_ASSERT_EQUAL(0, (int)st.st_size);

  DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::PRE_CLEAN);
  dtr->push(DataStaging::PRE_PROCESSOR);

  DataStaging::Processor processor;
  processor.start();
  //CPPUNIT_ASSERT(processor);
  processor.receiveDTR(*dtr);
  // sleep while thread deletes
  Arc::Time now;
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::PRE_CLEANED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::PRE_CLEANED, dtr->get_status().GetStatus());
  // check file is deleted
  CPPUNIT_ASSERT(!Arc::FileStat(destination, &st, true));

  // use a non-existent file
  destination = "http://badhost/file1";
  delete dtr;
  dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  dtr->set_status(DataStaging::DTRStatus::PRE_CLEAN);
  dtr->push(DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(*dtr);

  // sleep while thread deletes
  now = Arc::Time();
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::PRE_CLEANED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
  // PRE_CLEANED is the correct status even after an error
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::PRE_CLEANED, dtr->get_status().GetStatus());
  delete dtr;
}

void ProcessorTest::TestCacheCheck() {

  // create tmp cache dir for test
  std::string tmpdir;
  CPPUNIT_ASSERT(Arc::TmpDirCreate(tmpdir));

  std::string cache_dir(tmpdir);
  cache_dir += "/cache";
  DataStaging::CacheParameters cache_param;
  cache_param.cache_dirs.push_back(cache_dir);

  // use non-cacheable input and check it cannot be not cached
  std::string jobid("123456789");
  std::string source("http://www.nordugrid.org;cache=no/data/echo.sh");
  std::string destination("/tmp/file1");

  DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::NON_CACHEABLE);
  dtr->set_cache_parameters(cache_param);

  // use cacheable input
  source = "http://www.nordugrid.org/data/echo.sh";
  delete dtr;

  dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::CACHEABLE);
  dtr->set_cache_parameters(cache_param);
  dtr->set_status(DataStaging::DTRStatus::CHECK_CACHE);
  dtr->push(DataStaging::PRE_PROCESSOR);
  std::string cache_file(cache_dir + "/data/0c/4e5842a7c304ce2cd684631ca86bb366b43a7e");
  remove(cache_file.c_str());

  DataStaging::Processor processor;
  processor.start();
  //CPPUNIT_ASSERT(processor);
  processor.receiveDTR(*dtr);

  // sleep while thread checks cache
  Arc::Time now = Arc::Time();
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_CHECKED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);

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
  dtr->push(DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(*dtr);

  // sleep while thread checks cache
  now = Arc::Time();
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_WAIT) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_WAIT, dtr->get_status().GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::CACHE_LOCKED, dtr->get_cache_state());
  CPPUNIT_ASSERT_EQUAL(0, remove(lock_file.c_str()));

  // file present (need proxy for permission check)
  if (valid_proxy) {

    fd = ::open(cache_file.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    CPPUNIT_ASSERT(fd);
    char cache_file_contents[] = "abcde";
    CPPUNIT_ASSERT(write(fd, cache_file_contents, sizeof(cache_file_contents)) > 0);
    CPPUNIT_ASSERT_EQUAL(0, close(fd));

    dtr->set_status(DataStaging::DTRStatus::CHECK_CACHE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while thread checks cache
    now = Arc::Time();
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_CHECKED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);

    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_CHECKED, dtr->get_status().GetStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::CACHE_ALREADY_PRESENT, dtr->get_cache_state());
    CPPUNIT_ASSERT_EQUAL(0, remove(cache_file.c_str()));
  }

  // test files using guids are handled properly
  source = "lfc://lfc1.ndgf.org/:guid=4a2b61aa-1e57-4d32-9f23-873a9c9b9aed";
  delete dtr;

  dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::CACHEABLE);
  dtr->set_cache_parameters(cache_param);
  dtr->set_status(DataStaging::DTRStatus::CHECK_CACHE);
  dtr->push(DataStaging::PRE_PROCESSOR);
  cache_file = cache_dir + "/data/d0/a78f14e02ecff84cb5e20e5806ca99d536126b";
  remove(cache_file.c_str());

  processor.receiveDTR(*dtr);

  // sleep while thread checks cache
  now = Arc::Time();
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_CHECKED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_CHECKED, dtr->get_status().GetStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::CACHEABLE, dtr->get_cache_state());
  CPPUNIT_ASSERT_EQUAL(cache_file, dtr->get_cache_file());

  // clean up
  CPPUNIT_ASSERT(Arc::DirDelete(tmpdir));
}

void ProcessorTest::TestResolve() {

  DataStaging::Processor processor;
  processor.start();
  //CPPUNIT_ASSERT(processor);

  std::string filename = Arc::UUID();
  CPPUNIT_ASSERT(!filename.empty());
  std::string jobid("123456789");

  // resolve a good source
  std::string source("rls://rls.nordugrid.org/ABCDE");
  std::string destination("/tmp/file1");
  if (valid_proxy) {
    DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
    CPPUNIT_ASSERT(dtr);
    CPPUNIT_ASSERT(*dtr);
    dtr->set_status(DataStaging::DTRStatus::RESOLVE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while thread resolves
    Arc::Time now;
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

    // check that it found replicas
    CPPUNIT_ASSERT(dtr->get_source()->HaveLocations());
    delete dtr;
  }

  // pre-register a good destination
  source = "/tmp/file1";
  destination = "rls://gsiftp://some.host:2811/some/path@rls.nordugrid.org/" + filename;
  if (valid_proxy) {
    DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
    CPPUNIT_ASSERT(dtr);
    CPPUNIT_ASSERT(*dtr);
    dtr->set_status(DataStaging::DTRStatus::RESOLVE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while thread resolves
    Arc::Time now;
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

    // check that it added the destination replica
    CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
    CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://some.host:2811/some/path/"+filename), dtr->get_destination()->CurrentLocation().str());

    // would like check the LFN exists in the index service
    // but RLS doesn't support LFN without PFN so we'll check this in
    // the test for REGISTER_REPLICA
    /*
    std::list<Arc::FileInfo> files;
    CPPUNIT_ASSERT(dtr->get_destination()->ListFiles(files));
    CPPUNIT_ASSERT_EQUAL(1, (int)files.size());
    CPPUNIT_ASSERT_EQUAL(std::string("rls://rls.nordugrid.org/"+filename), files.front().GetName());
    */
    delete dtr;
  }

  // test replication
  source = "rls://rls.nordugrid.org/ABCDE";
  destination = "rls://gsiftp://some.host:2811/some/path@rls.nordugrid.org/ABCDE";
  if (valid_proxy) {
    DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
    CPPUNIT_ASSERT(dtr);
    CPPUNIT_ASSERT(*dtr);
    dtr->set_status(DataStaging::DTRStatus::RESOLVE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    dtr->set_replication(true); // usually set automatically by scheduler
    processor.receiveDTR(*dtr);

    // sleep while thread resolves
    Arc::Time now;
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

    // check that it found replicas
    CPPUNIT_ASSERT(dtr->get_source()->HaveLocations());

    // check that it added the destination replica
    CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
    CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://some.host:2811/some/path/ABCDE"), dtr->get_destination()->CurrentLocation().str());

    delete dtr;
  }

  // copy to an existing LFN from a different LFN
  source = "/tmp/1.1";
  destination = "rls://gsiftp://some.host:2811/some/path@rls.nordugrid.org/ABCDE";
  if (valid_proxy) {
    DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
    CPPUNIT_ASSERT(dtr);
    CPPUNIT_ASSERT(*dtr);
    dtr->set_status(DataStaging::DTRStatus::RESOLVE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while thread resolves
    Arc::Time now;
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);
    // will fail since force_registration is not set
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

    // set force registration and try again
    dtr->set_force_registration(true);
    dtr->reset_error_status();
    dtr->set_status(DataStaging::DTRStatus::RESOLVE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while thread resolves
    now = Arc::Time();
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);
    // should be successful now
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());

    // check that it added the destination replica
    CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
    CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://some.host:2811/some/path/ABCDE"), dtr->get_destination()->CurrentLocation().str());
    delete dtr;
  }

  // source and destination same file - should fail
  source = "rls://rls.nordugrid.org/ABCDE";
  destination = "rls://https://knowarc1.grid.niif.hu:8001/se@rls.nordugrid.org/ABCDE";
  if (valid_proxy) {
    DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
    CPPUNIT_ASSERT(dtr);
    CPPUNIT_ASSERT(*dtr);
    dtr->set_status(DataStaging::DTRStatus::RESOLVE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while thread resolves
    Arc::Time now;
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);
    // will fail with self-replication error
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::SELF_REPLICATION_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());
    delete dtr;
  }
}

void ProcessorTest::TestQueryReplica() {

  // query a valid file
  std::string jobid("123456789");
  std::string source("http://www.nordugrid.org/data/echo.sh");
  std::string destination("/tmp/file1");

  DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);

  DataStaging::Processor processor;
  processor.start();
  //CPPUNIT_ASSERT(processor);
  dtr->set_status(DataStaging::DTRStatus::QUERY_REPLICA);
  dtr->push(DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(*dtr);

  // sleep while replica is queried
  Arc::Time now;
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::REPLICA_QUERIED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::REPLICA_QUERIED, dtr->get_status().GetStatus());

  // invalid file
  source = "http://www.nordugrid.org/bad/dir/badfile";
  delete dtr;
  dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);

  dtr->set_status(DataStaging::DTRStatus::QUERY_REPLICA);
  dtr->push(DataStaging::PRE_PROCESSOR);
  processor.receiveDTR(*dtr);

  // sleep while replica is queried
  now = Arc::Time();
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::REPLICA_QUERIED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::REPLICA_QUERIED, dtr->get_status().GetStatus());

  // index file with inconsistent metadata - needs proxy
  if (valid_proxy) {
    source = "rls://rls.nordugrid.org/processor_test_inconsistent_metadata";
    delete dtr;
    dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
    CPPUNIT_ASSERT(dtr);
    CPPUNIT_ASSERT(*dtr);

    dtr->reset_error_status();

    // first resolve replicas
    dtr->set_status(DataStaging::DTRStatus::RESOLVE);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while replicas are resolved
    now = Arc::Time();
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::RESOLVED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);

    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::RESOLVED, dtr->get_status().GetStatus());
    dtr->set_status(DataStaging::DTRStatus::QUERY_REPLICA);
    dtr->push(DataStaging::PRE_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while replica is queried
    now = Arc::Time();
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::REPLICA_QUERIED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);

    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::PERMANENT_REMOTE_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::REPLICA_QUERIED, dtr->get_status().GetStatus());
  }
  delete dtr;
}

void ProcessorTest::TestReplicaRegister() {

  DataStaging::Processor processor;
  processor.start();
  //CPPUNIT_ASSERT(processor);

  std::string filename = Arc::UUID();
  CPPUNIT_ASSERT(!filename.empty());
  std::string jobid("123456789");

  // register a file
  std::string source("/tmp/file1");
  std::string destination("rls://gsiftp://some.host:2811/some/path@rls.nordugrid.org/" + filename);
  if (valid_proxy) {
    DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
    CPPUNIT_ASSERT(dtr);
    CPPUNIT_ASSERT(*dtr);

    // have to resolve first
    CPPUNIT_ASSERT(dtr->get_destination()->Resolve(false).Passed());
    CPPUNIT_ASSERT(dtr->get_destination()->HaveLocations());
    CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://some.host:2811/some/path/"+filename), dtr->get_destination()->CurrentLocation().str());

    // pre-register
    CPPUNIT_ASSERT(dtr->get_destination()->PreRegister(false, false).Passed());
    // with RLS this doesn't add LFN so we can't check existence yet

    // post-register
    dtr->set_status(DataStaging::DTRStatus::REGISTER_REPLICA);
    dtr->push(DataStaging::POST_PROCESSOR);
    processor.receiveDTR(*dtr);

    // sleep while thread resgisters
    Arc::Time now;
    while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::REPLICA_REGISTERED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
      usleep(100);
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
    CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::REPLICA_REGISTERED, dtr->get_status().GetStatus());

    // check registration is ok
    Arc::FileInfo file;
    Arc::DataPoint::DataPointInfoType verb = (Arc::DataPoint::DataPointInfoType)(Arc::DataPoint::INFO_TYPE_CONTENT | Arc::DataPoint::INFO_TYPE_STRUCT);
    CPPUNIT_ASSERT(dtr->get_destination()->Stat(file, verb).Passed());
    std::list<Arc::URL> replicas = file.GetURLs();
    CPPUNIT_ASSERT_EQUAL(1, (int)replicas.size());
    CPPUNIT_ASSERT_EQUAL(std::string("gsiftp://some.host:2811/some/path/"+filename), replicas.front().str());

    // clean up
    CPPUNIT_ASSERT(dtr->get_destination()->Unregister(true).Passed());
    delete dtr;
  }

  // can't check request error condition with RLS - unregistering file has no effect
  // neither does registering a file that wasn't pre-registered
}

void ProcessorTest::TestCacheProcess() {

  std::string tmpdir;
  CPPUNIT_ASSERT(Arc::TmpDirCreate(tmpdir));

  std::string session(tmpdir);
  session += "/session";
  std::string cache_dir(tmpdir);
  cache_dir += "/cache";
  DataStaging::CacheParameters cache_param;
  cache_param.cache_dirs.push_back(cache_dir);

  // make conf file
  // TODO Pass conf file to data staging. Until then this test will fail!!!
  std::string conf_file(tmpdir);
  conf_file += "/arc.conf";
  int fd = ::open(conf_file.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
  CPPUNIT_ASSERT(fd);
  std::string conf("[grid-manager]\ncache=\"" + cache_dir + "\"\n");
  CPPUNIT_ASSERT(write(fd, conf.c_str(), conf.length()) > 0);
  CPPUNIT_ASSERT_EQUAL(0, close(fd));

  std::string jobid("123456789");
  std::string source("http://www.nordugrid.org/data/echo.sh");
  std::string destination(std::string(session+"/file1"));

  DataStaging::DTR *dtr = new DataStaging::DTR(source, destination, *cfg, jobid, Arc::User().get_uid(), logger);
  CPPUNIT_ASSERT(dtr);
  CPPUNIT_ASSERT(*dtr);
  CPPUNIT_ASSERT(dtr->get_cache_state() == DataStaging::CACHEABLE);
  dtr->set_cache_parameters(cache_param);

  // process with no cache file present
  std::string cache_file(cache_dir + "/data/0c/4e5842a7c304ce2cd684631ca86bb366b43a7e");
  remove(cache_file.c_str());

  DataStaging::Processor processor;
  processor.start();
  //CPPUNIT_ASSERT(processor);
  dtr->set_status(DataStaging::DTRStatus::PROCESS_CACHE);
  dtr->push(DataStaging::POST_PROCESSOR);
  processor.receiveDTR(*dtr);

  // sleep while cache is processed
  Arc::Time now;
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_PROCESSED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::CACHE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_PROCESSED, dtr->get_status().GetStatus());

  // create cache file and try again
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(cache_dir+"/data/0c"), 0700, true));
  fd = ::open(cache_file.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
  CPPUNIT_ASSERT(fd);
  char cache_file_contents[] = "abcde";
  CPPUNIT_ASSERT(write(fd, cache_file_contents, sizeof(cache_file_contents)) > 0);
  CPPUNIT_ASSERT_EQUAL(0, close(fd));

  dtr->reset_error_status();
  dtr->set_status(DataStaging::DTRStatus::PROCESS_CACHE);
  dtr->push(DataStaging::POST_PROCESSOR);
  processor.receiveDTR(*dtr);

  // sleep while cache is processed
  now = Arc::Time();
  while ((dtr->get_status().GetStatus() != DataStaging::DTRStatus::CACHE_PROCESSED) && ((Arc::Time() - now) <= CONNECTION_TIMEOUT))
    usleep(100);

  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRErrorStatus::NONE_ERROR, dtr->get_error_status().GetErrorStatus());
  CPPUNIT_ASSERT_EQUAL(DataStaging::DTRStatus::CACHE_PROCESSED, dtr->get_status().GetStatus());

  // check correct links exist
  struct stat st;
  CPPUNIT_ASSERT_EQUAL(0, stat(std::string(cache_dir + "/joblinks/123456789/file1").c_str(), &st));
  CPPUNIT_ASSERT_EQUAL(0, stat(std::string(session + "/file1").c_str(), &st));

  // clean up
  CPPUNIT_ASSERT(Arc::DirDelete(tmpdir));
}



CPPUNIT_TEST_SUITE_REGISTRATION(ProcessorTest);
