#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <utime.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include <arc/FileLock.h>

class FileLockTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileLockTest);
  CPPUNIT_TEST(TestFileLockAcquire);
  CPPUNIT_TEST(TestFileLockRelease);
  CPPUNIT_TEST(TestFileLockCheck);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestFileLockAcquire();
  void TestFileLockRelease();
  void TestFileLockCheck();

private:
  bool _createFile(const std::string& filename, const std::string& text = "a");
  std::string _readFile(const std::string& filename);
  std::string testroot;
};

void FileLockTest::setUp() {
  std::string tmpdir;
  Arc::TmpDirCreate(tmpdir);
  testroot = tmpdir;
}

void FileLockTest::tearDown() {
  Arc::DirDelete(testroot);
}

void FileLockTest::TestFileLockAcquire() {
  // test that a lock can be acquired
  std::string filename(testroot + "/file1");
  std::string lock_file(filename + ".lock");

  Arc::FileLock lock(filename);
  CPPUNIT_ASSERT(lock.acquire());

  // test file is locked
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));

  // look at modification time - should not be more than 1 second old
  time_t mod_time = fileStat.st_mtime;
  time_t now = time(NULL);
  CPPUNIT_ASSERT((now - mod_time) <= 1);

  // check it has the right pid inside
  std::string lock_pid = _readFile(lock_file);
  CPPUNIT_ASSERT(lock_pid != "");

  // construct hostname
  char hostname[256];
  if(gethostname(hostname, sizeof(hostname)) != 0) hostname[0] = '\0';
  std::string host(hostname);
  CPPUNIT_ASSERT_EQUAL(Arc::tostring(getpid()) + "@" + host, lock_pid);

  bool lock_removed = false;
  struct utimbuf times;
  time_t t = 1;
  // set old modification time
  // utime does not work on windows - skip for windows
  times.actime = t;
  times.modtime = t;
  CPPUNIT_ASSERT_EQUAL(0, utime(lock_file.c_str(), &times));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  CPPUNIT_ASSERT_EQUAL(t, fileStat.st_mtime);

  // call acquire() again - should succeed and make new lock file
  CPPUNIT_ASSERT(lock.acquire(lock_removed));
  CPPUNIT_ASSERT(lock_removed);

  // look at modification time - should not be more than 1 second old
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  mod_time = fileStat.st_mtime;
  now = time(NULL);
  CPPUNIT_ASSERT((now - mod_time) <= 1);

  // lock the file with a pid which is still running on this host
  _createFile(lock_file, "1@" + host);
  lock_removed = false;
  CPPUNIT_ASSERT(!lock.acquire(lock_removed));
  CPPUNIT_ASSERT(!lock_removed);

  // lock with process on different host
  lock_removed = false;
  _createFile(lock_file, "1@mybadhost.org");
  CPPUNIT_ASSERT(!lock.acquire(lock_removed));
  CPPUNIT_ASSERT(!lock_removed);

  // try again with a non-existent pid
  // windows can't check for running pid - skip for windows
  _createFile(lock_file, "99999@" + host);
  lock_removed = false;
  CPPUNIT_ASSERT(lock.acquire(lock_removed));
  CPPUNIT_ASSERT(lock_removed);

  // badly formatted pid
  _createFile(lock_file, "abcd@" + host);
  lock_removed = false;
  CPPUNIT_ASSERT(!lock.acquire(lock_removed));
  CPPUNIT_ASSERT(!lock_removed);

  // set small timeout
  CPPUNIT_ASSERT_EQUAL(0, remove(lock_file.c_str()));
  lock_removed = false;
  lock = Arc::FileLock(filename, 1);
  CPPUNIT_ASSERT(lock.acquire(lock_removed));
  CPPUNIT_ASSERT(!lock_removed);
  // use longer sleep because times and sleeps are very
  // approximate on windows
  sleep(4);
  CPPUNIT_ASSERT(lock.acquire(lock_removed));
  CPPUNIT_ASSERT(lock_removed);

  // don't use pid
  CPPUNIT_ASSERT_EQUAL(0, remove(lock_file.c_str()));
  lock_removed = false;
  lock = Arc::FileLock(filename, 30, false);
  CPPUNIT_ASSERT(lock.acquire(lock_removed));
  CPPUNIT_ASSERT(!lock_removed);

  // check lock file is empty
  lock_pid = _readFile(lock_file);
  CPPUNIT_ASSERT(lock_pid.empty());

  // create an empty lock file - acquire should fail
  CPPUNIT_ASSERT_EQUAL(0, remove(lock_file.c_str()));
  _createFile(lock_file, "");
  lock = Arc::FileLock(filename);
  CPPUNIT_ASSERT(!lock.acquire());

  // set old modification time - acquire should now succeed
  // utime does not work on windows - skip for windows
  times.actime = t;
  times.modtime = t;
  CPPUNIT_ASSERT_EQUAL(0, utime(lock_file.c_str(), &times));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  CPPUNIT_ASSERT_EQUAL(t, fileStat.st_mtime);

  CPPUNIT_ASSERT(lock.acquire());

  // create lock with empty hostname - acquire should still work
  // windows can't check for running pid - skip for windows
  lock = Arc::FileLock(filename);
  _createFile(lock_file, "99999@");
  lock_removed = false;
  CPPUNIT_ASSERT(lock.acquire(lock_removed));
  CPPUNIT_ASSERT(lock_removed);
}

void FileLockTest::TestFileLockRelease() {

  std::string filename(testroot + "/file2");
  std::string lock_file(filename + ".lock");

  // release non-existent lock
  struct stat fileStat;
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);
  Arc::FileLock lock(filename);
  CPPUNIT_ASSERT(!lock.release());

  // construct hostname
  char hostname[256];
  if(gethostname(hostname, sizeof(hostname)) != 0) hostname[0] = '\0';
  std::string host(hostname);

  // create a valid lock file with this pid
  _createFile(lock_file, std::string(Arc::tostring(getpid()) + "@" + host));

  lock = Arc::FileLock(filename);
  CPPUNIT_ASSERT(lock.release());

  // test lock file is gone
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);

  // create a lock with a different pid
  _createFile(lock_file, std::string("1@" + host));
  CPPUNIT_ASSERT(!lock.release());

  // test lock file is still there
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));

  // create lock with different host
  CPPUNIT_ASSERT_EQUAL(0, remove(lock_file.c_str()));
  _createFile(lock_file, std::string(Arc::tostring(getpid()) + "@mybadhost.org"));
  CPPUNIT_ASSERT(!lock.release());
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));

  // force release
  CPPUNIT_ASSERT(lock.release(true));
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);

  // create an empty lock file - release should fail
  _createFile(lock_file, "");
  lock = Arc::FileLock(filename);
  CPPUNIT_ASSERT(!lock.release());

  // set use_pid false, release should succeed now
  lock = Arc::FileLock(filename, 30, false);
  CPPUNIT_ASSERT(lock.release());

  // create lock with empty hostname - release should still work
  lock = Arc::FileLock(filename);
  _createFile(lock_file, std::string(Arc::tostring(getpid()) + "@"));
  CPPUNIT_ASSERT(lock.release());

}

void FileLockTest::TestFileLockCheck() {

  std::string filename(testroot + "/file3");
  std::string lock_file(filename + ".lock");

  // check non-existent lock
  struct stat fileStat;
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);
  Arc::FileLock lock(filename);
  CPPUNIT_ASSERT_EQUAL(-1, lock.check());

  // construct hostname
  char hostname[256];
  if(gethostname(hostname, sizeof(hostname)) != 0) hostname[0] = '\0';
  std::string host(hostname);

  // create a valid lock file with this pid
  _createFile(lock_file, std::string(Arc::tostring(getpid()) + "@" + host));

  lock = Arc::FileLock(filename);
  CPPUNIT_ASSERT_EQUAL(0, lock.check());

  // create a lock with a different pid
  _createFile(lock_file, std::string("1@" + host));
  CPPUNIT_ASSERT_EQUAL(1, lock.check());

  // create lock with different host
  CPPUNIT_ASSERT_EQUAL(0, remove(lock_file.c_str()));
  _createFile(lock_file, std::string(Arc::tostring(getpid()) + "@mybadhost.org"));
  CPPUNIT_ASSERT_EQUAL(-1, lock.check());

  // create an empty lock file - check should fail
  _createFile(lock_file, "");
  lock = Arc::FileLock(filename);
  CPPUNIT_ASSERT_EQUAL(-1, lock.check());

  // set use_pid false, check should succeed now
  lock = Arc::FileLock(filename, 30, false);
  CPPUNIT_ASSERT_EQUAL(0, lock.check());

  // create lock with empty hostname - check should still be ok
  lock = Arc::FileLock(filename);
  _createFile(lock_file, std::string(Arc::tostring(getpid()) + "@"));
  CPPUNIT_ASSERT_EQUAL(0, lock.check());
}

bool FileLockTest::_createFile(const std::string& filename, const std::string& text) {

  remove(filename.c_str());
  FILE *pFile;
  pFile = fopen((char*)filename.c_str(), "w");
  if (pFile == NULL) return false;
  fputs((char*)text.c_str(), pFile);
  fclose(pFile);
  return true;
}

std::string FileLockTest::_readFile(const std::string& filename) {

  FILE *pFile;
  char mystring[1024];
  pFile = fopen((char*)filename.c_str(), "r");
  if (pFile == NULL)
    return "";
  std::string data;
  while (fgets(mystring, sizeof(mystring), pFile))
    data += std::string(mystring);
  fclose(pFile);

  return data;
}

CPPUNIT_TEST_SUITE_REGISTRATION(FileLockTest);
