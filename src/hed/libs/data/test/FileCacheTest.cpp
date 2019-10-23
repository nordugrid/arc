// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <cppunit/extensions/HelperMacros.h>

#include <cerrno>
#include <list>

#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include <arc/FileAccess.h>

#include "../FileCache.h"

class FileCacheTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileCacheTest);
  CPPUNIT_TEST(testStart);
  CPPUNIT_TEST(testStop);
  CPPUNIT_TEST(testStopAndDelete);
  CPPUNIT_TEST(testLinkFile);
  CPPUNIT_TEST(testLinkFileLinkCache);
  CPPUNIT_TEST(testCopyFile);
  CPPUNIT_TEST(testFile);
  CPPUNIT_TEST(testRelease);
  CPPUNIT_TEST(testCheckDN);
  CPPUNIT_TEST(testTwoCaches);
  CPPUNIT_TEST(testCreationDate);
  CPPUNIT_TEST(testConstructor);
  CPPUNIT_TEST(testBadConstructor);
  CPPUNIT_TEST(testInternal);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void testStart();
  void testRemoteCache();
  void testRemoteCacheValidLock();
  void testRemoteCacheInvalidLock();
  void testRemoteCacheReplication();
  void testStop();
  void testStopAndDelete();
  void testLinkFile();
  void testLinkFileLinkCache();
  void testCopyFile();
  void testFile();
  void testRelease();
  void testCheckDN();
  void testTwoCaches();
  void testCreationDate();
  void testConstructor();
  void testBadConstructor();
  void testInternal();

private:
  std::string _testroot;
  std::string _cache_dir;
  std::string _cache_data_dir;
  std::string _cache_job_dir;
  std::string _session_dir;
  std::string _url;
  std::string _jobid;
  uid_t _uid;
  gid_t _gid;
  std::string _hostname;
  Arc::FileCache *_fc1;
  /** Create a file with the given size */
  bool _createFile(std::string filename, std::string text = "a");
  /** Return the contents of the given file */
  std::string _readFile(std::string filename);
};

void FileCacheTest::setUp() {

  std::string tmpdir;
  Arc::TmpDirCreate(tmpdir);
  _testroot = tmpdir;

  _cache_dir = _testroot + "/cache";
  _cache_data_dir = _cache_dir + "/data";
  _cache_job_dir = _cache_dir + "/joblinks";
  _session_dir = _testroot + "/session";

  _url = "http://host.org/file1";
  _uid = getuid();
  _gid = getgid();
  _jobid = "1";
  _fc1 = new Arc::FileCache(_cache_dir, _jobid, _uid, _gid);

  // construct hostname
  struct utsname buf;
  CPPUNIT_ASSERT_EQUAL(uname(&buf), 0);
  _hostname = buf.nodename;
}

void FileCacheTest::tearDown() {

  delete _fc1;
  Arc::DirDelete(_testroot);
}

void FileCacheTest::testStart() {

  // test that the cache Starts ok
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // test cache file is locked
  std::string lock_file(_fc1->File(_url) + ".lock");
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));

  // test meta file exists and contains correct url
  std::string meta_file(_fc1->File(_url) + ".meta");
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));
  std::string meta_url = _readFile(meta_file);
  CPPUNIT_ASSERT(meta_url != "");
  CPPUNIT_ASSERT_EQUAL(std::string(_url) + '\n', meta_url);

  // test no lock is left on the .meta file
  std::string meta_lock(meta_file + ".lock");
  CPPUNIT_ASSERT(stat(meta_lock.c_str(), &fileStat) != 0);

  // look at lock modification time - should not be more than 1 second old
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  time_t mod_time = fileStat.st_mtime;
  time_t now = time(NULL);
  CPPUNIT_ASSERT((now - mod_time) <= 1);

  // check it has the right pid inside
  std::string lock_pid = _readFile(lock_file);
  CPPUNIT_ASSERT(lock_pid != "");

  CPPUNIT_ASSERT_EQUAL(Arc::tostring(getpid()) + "@" + _hostname, lock_pid);

  // set old modification time
  struct utimbuf times;
  time_t t = 1;
  times.actime = t;
  times.modtime = t;
  CPPUNIT_ASSERT_EQUAL(0, utime(lock_file.c_str(), &times));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  CPPUNIT_ASSERT_EQUAL(t, fileStat.st_mtime);

  // call Start again - should succeed and make new lock file
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // look at lock modification time - should not be more than 1 second old
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  mod_time = fileStat.st_mtime;
  now = time(NULL);
  CPPUNIT_ASSERT((now - mod_time) <= 1);

  // create a cache file
  _createFile(_fc1->File(_url));

  // Stop cache
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  // check lock is released
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);
  // check meta file is still there
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));

  // call Start again and check available is true
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(available);
  CPPUNIT_ASSERT(!is_locked);

  // check no lock exists
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);

  // force delete - file should be unavailable and locked
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked, true));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // force delete again - should have same result
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked, true));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat lock file " + lock_file, 0, stat(lock_file.c_str(), &fileStat));
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // lock the file with a pid which is still running on this host and check is_locked
  _createFile(_fc1->File(_url) + ".lock", "1@" + _hostname);
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(is_locked);

  // lock with process on different host
  is_locked = false;
  _createFile(_fc1->File(_url) + ".lock", "1@mybadhost.org");
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(is_locked);

  // delete lock file and try again with a non-existent pid
  CPPUNIT_ASSERT_EQUAL(0, remove(std::string(_fc1->File(_url) + ".lock").c_str()));
  _createFile(_fc1->File(_url) + ".lock", "99999@" + _hostname);
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // put different url in meta file
  _createFile(_fc1->File(_url) + ".meta", "http://badfile");
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));

  // locked meta file - this is ok and will be ignored
  _createFile(_fc1->File(_url) + ".meta", _url);
  _createFile(meta_lock, "1@" + _hostname);
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // remove meta file - now locked should be true
  CPPUNIT_ASSERT_EQUAL(0, remove(meta_file.c_str()));
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(is_locked);
  CPPUNIT_ASSERT_EQUAL(0, remove(meta_lock.c_str()));

  // empty meta file - should be recreated
  _createFile(_fc1->File(_url) + ".meta", "");
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));
  CPPUNIT_ASSERT_EQUAL(0, (int)fileStat.st_size);
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));
  CPPUNIT_ASSERT(fileStat.st_size > 0);

  // corrupted meta file - should be recreated
  _createFile(_fc1->File(_url) + ".meta", "\n");
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));
  CPPUNIT_ASSERT_EQUAL(1, (int)fileStat.st_size);
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));
  CPPUNIT_ASSERT(fileStat.st_size > 1);

  // Use a bad cache dir
  if (_uid != 0 && stat("/lost+found/cache", &fileStat) != 0 && errno == EACCES) {
    std::vector<std::string> caches;
    caches.push_back("/lost+found/cache");
    delete _fc1;
    _fc1 = new Arc::FileCache(caches, _jobid, _uid, _gid);
    CPPUNIT_ASSERT(*_fc1);
    CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
    CPPUNIT_ASSERT(!available);
    CPPUNIT_ASSERT(!is_locked);
    // with one good dir and one bad dir Start should succeed
    caches.push_back(_cache_dir);
    delete _fc1;
    _fc1 = new Arc::FileCache(caches, _jobid, _uid, _gid);
    CPPUNIT_ASSERT(*_fc1);
    CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
    CPPUNIT_ASSERT(!available);
    CPPUNIT_ASSERT(!is_locked);
  }
}

void FileCacheTest::testStop() {

  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  std::string cache_file(_fc1->File(_url));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // Stop cache with non-existent cache file - this is ok
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  // check lock is released
  std::string lock_file(cache_file + ".lock");
  struct stat fileStat;
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);

  // Start again to create a new lock
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);

  // create the cache file
  _createFile(cache_file);
  // Stop cache
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // check lock is released
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);

  // check cache file is still there
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat cache file " + cache_file, 0, stat(cache_file.c_str(), &fileStat));

  // check meta file is still there with correct url
  std::string meta_file(cache_file + ".meta");
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));
  std::string meta_url = _readFile(meta_file);
  CPPUNIT_ASSERT(meta_url != "");
  CPPUNIT_ASSERT_EQUAL(std::string(_url) + '\n', meta_url);

  // call with non-existent lock file
  CPPUNIT_ASSERT(!_fc1->Stop(_url));

  // put different pid in lock file - Stop() should return false
  _createFile(lock_file, Arc::tostring(getpid() + 1));
  CPPUNIT_ASSERT(!_fc1->Stop(_url));

  // check that lock and cache file are still there
  CPPUNIT_ASSERT(stat(cache_file.c_str(), &fileStat) == 0);
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) == 0);

  // check meta file is still there
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat meta file " + meta_file, 0, stat(meta_file.c_str(), &fileStat));
}

void FileCacheTest::testStopAndDelete() {

  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  std::string cache_file(_fc1->File(_url));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // Stop and delete with non-existent cache file
  CPPUNIT_ASSERT(_fc1->StopAndDelete(_url));
  // check lock is released
  std::string lock_file(cache_file + ".lock");
  struct stat fileStat;
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);

  // check meta file is deleted
  std::string meta_file(cache_file + ".meta");
  CPPUNIT_ASSERT(stat(meta_file.c_str(), &fileStat) != 0);

  // Start again to create a new lock
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);

  // create the cache file
  CPPUNIT_ASSERT(_createFile(cache_file));
  // check cache file is there
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat cache file " + cache_file, 0, stat(cache_file.c_str(), &fileStat));

  // Stop cache and delete file
  CPPUNIT_ASSERT(_fc1->StopAndDelete(_url));

  // check lock is released
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) != 0);

  // check cache file has gone
  CPPUNIT_ASSERT(stat(cache_file.c_str(), &fileStat) != 0);

  // check meta file is deleted
  CPPUNIT_ASSERT(stat(meta_file.c_str(), &fileStat) != 0);

  // create the cache file
  CPPUNIT_ASSERT(_createFile(cache_file));

  // call with non-existent lock file
  CPPUNIT_ASSERT(!_fc1->StopAndDelete(_url));

  // check the cache file is still there
  CPPUNIT_ASSERT(stat(cache_file.c_str(), &fileStat) == 0);

  // put different pid in lock file - StopAndDelete() should return false
  _createFile(lock_file, Arc::tostring(getpid() + 1));
  CPPUNIT_ASSERT(!_fc1->StopAndDelete(_url));

  // check that lock and cache files are still there
  CPPUNIT_ASSERT(stat(cache_file.c_str(), &fileStat) == 0);
  CPPUNIT_ASSERT(stat(lock_file.c_str(), &fileStat) == 0);

}

void FileCacheTest::testLinkFile() {

  std::string soft_link(_session_dir + "/" + _jobid + "/file1");
  std::string hard_link(_cache_job_dir + "/" + _jobid + "/file1");
  bool try_again = false;
  // link non-existent file - should return false and set try_again true
  CPPUNIT_ASSERT(!_fc1->Link(soft_link, _url, false, false, false, try_again));
  CPPUNIT_ASSERT(try_again);

  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));

  // check cache file is there
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat cache file " + _fc1->File(_url), 0, stat(_fc1->File(_url).c_str(), &fileStat));

  // create link
  CPPUNIT_ASSERT(_fc1->Link(soft_link, _url, false, false, true, try_again));

  // check hard- and soft-links exist
  CPPUNIT_ASSERT(stat((_cache_job_dir + "/1").c_str(), &fileStat) == 0);
  CPPUNIT_ASSERT((fileStat.st_mode & S_IRWXU) == S_IRWXU);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat hard link " + hard_link, 0, stat(hard_link.c_str(), &fileStat));
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat soft link " + soft_link, 0, stat(soft_link.c_str(), &fileStat));

  // create bad soft-link
  if (_uid != 0 && stat("/lost+found/sessiondir", &fileStat) != 0 && errno == EACCES) {
    CPPUNIT_ASSERT(!_fc1->Link("/lost_found/sessiondir/file1", _url, false, false, false, try_again));
    CPPUNIT_ASSERT(!try_again);
  }

  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // lock with valid lock
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)+".lock", std::string("1@" + _hostname)));
  // link should fail and links should be cleaned up
  CPPUNIT_ASSERT(!_fc1->Link(soft_link, _url, false, false, false, try_again));
  CPPUNIT_ASSERT(try_again);
  CPPUNIT_ASSERT(stat(hard_link.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT(stat(soft_link.c_str(), &fileStat) != 0);
}

void FileCacheTest::testLinkFileLinkCache() {

  // new cache with link path set
  std::string cache_link_dir = _testroot + "/link";
  _fc1 = new Arc::FileCache(_cache_dir + " " + cache_link_dir, _jobid, _uid, _gid);
  CPPUNIT_ASSERT(symlink(_cache_dir.c_str(), cache_link_dir.c_str()) == 0);

  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));

  // check cache file is there
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat cache file " + _fc1->File(_url), 0, stat(_fc1->File(_url).c_str(), &fileStat));

  // create link
  std::string soft_link = _session_dir + "/" + _jobid + "/file1";
  std::string hard_link = _cache_job_dir + "/" + _jobid + "/file1";
  bool try_again = false;
  CPPUNIT_ASSERT(_fc1->Link(soft_link, _url, false, false, true, try_again));

  // check soft link is ok and points to the right place
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat soft link " + soft_link, 0, lstat(soft_link.c_str(), &fileStat));
  // check our soft link links to yet another link
  CPPUNIT_ASSERT(S_ISLNK(fileStat.st_mode));
  // check the target is correct
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat target of soft link " + soft_link, 0, stat(soft_link.c_str(), &fileStat));

  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));
}

void FileCacheTest::testCopyFile() {

  // TODO integrate into testLinkFile()
  std::string dest_file = _session_dir + "/" + _jobid + "/file1";
  std::string hard_link = _cache_job_dir + "/" + _jobid + "/file1";
  bool try_again = false;

  // copy non-existent file
  CPPUNIT_ASSERT(!_fc1->Link(dest_file, _url, true, false, false, try_again));
  CPPUNIT_ASSERT(try_again);

  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));

  // check cache file is there
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat cache file " + _fc1->File(_url), 0, stat(_fc1->File(_url).c_str(), &fileStat));

  // do copy
  // NOTE: not possible to test executable since can't use FileAccess directly during tests
  CPPUNIT_ASSERT(_fc1->Link(dest_file, _url, true, false, true, try_again));

  // check copy exists and is executable
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat destination file " + dest_file, 0, stat(dest_file.c_str(), &fileStat));

  // create bad copy
  if (_uid != 0 && stat("/lost+found/sessiondir", &fileStat) != 0 && errno == EACCES)
    CPPUNIT_ASSERT(!_fc1->Link("/lost+found/sessiondir/file1", _url, true, false, false, try_again));

  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));
}

void FileCacheTest::testFile() {
  // test hash returned
  std::string hash = "/8a/929b8384300813ba1dd2d661c42835b80691a2";
  CPPUNIT_ASSERT_EQUAL(std::string(_cache_data_dir + hash), _fc1->File(_url));

  // set up two caches
  std::vector<std::string> caches;
  caches.push_back(_cache_dir);
  std::string cache_dir2 = _cache_dir + "2";
  caches.push_back(cache_dir2);

  Arc::FileCache *fc2 = new Arc::FileCache(caches, "1", _uid, _gid);
  // _url can go to either cache, so check it at least is mapped to one
  bool found = false;
  if (fc2->File(_url) == std::string(_cache_data_dir + hash))
    found = true;
  if (fc2->File(_url) == std::string(cache_dir2 + "/data" + hash))
    found = true;
  
  CPPUNIT_ASSERT(found);
  delete fc2;
}

void FileCacheTest::testRelease() {

  // create cache and link
  std::string soft_link = _session_dir + "/" + _jobid + "/file1";
  std::string hard_link = _cache_job_dir + "/" + _jobid + "/file1";
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));

  // create link
  bool try_again = false;
  CPPUNIT_ASSERT(_fc1->Link(soft_link, _url, false, false, true, try_again));

  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // release
  CPPUNIT_ASSERT(_fc1->Release());

  // check files and dir are gone
  struct stat fileStat;
  CPPUNIT_ASSERT(stat(hard_link.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT(stat(std::string(_cache_job_dir + "/" + _jobid).c_str(), &fileStat) != 0);

  CPPUNIT_ASSERT_EQUAL(0, remove(soft_link.c_str()));
  // start again but don't create links
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // check link dirs are not there
  CPPUNIT_ASSERT(stat(hard_link.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT(stat(std::string(_cache_job_dir + "/" + _jobid).c_str(), &fileStat) != 0);

  // release should not give an error, even though job dir does not exist
  CPPUNIT_ASSERT(_fc1->Release());
}

void FileCacheTest::testCheckDN() {

  // Bad DN
  CPPUNIT_ASSERT(!_fc1->AddDN(_url, "", Arc::Time()));
  CPPUNIT_ASSERT(!_fc1->CheckDN(_url, ""));

  std::string dn1 = "/O=Grid/O=NorduGrid/OU=test.org/CN=Mr Tester";

  // non-existent meta file
  CPPUNIT_ASSERT(!_fc1->AddDN(_url, dn1, Arc::Time()));
  CPPUNIT_ASSERT(!_fc1->CheckDN(_url, dn1));

  // create empty meta file
  std::string meta_file = _fc1->File(_url) + ".meta";
  CPPUNIT_ASSERT(_createFile(meta_file, ""));
  CPPUNIT_ASSERT(!_fc1->AddDN(_url, dn1, Arc::Time()));
  CPPUNIT_ASSERT(!_fc1->CheckDN(_url, dn1));

  // create proper meta file
  CPPUNIT_ASSERT(_createFile(meta_file, _url + '\n'));
  CPPUNIT_ASSERT(!_fc1->CheckDN(_url, dn1));

  // add DN
  Arc::Time now = Arc::Time();
  Arc::Time futuretime = Arc::Time(now.GetTime() + 1000);
  CPPUNIT_ASSERT(_fc1->AddDN(_url, dn1, futuretime));
  CPPUNIT_ASSERT(_fc1->CheckDN(_url, dn1));
  CPPUNIT_ASSERT_EQUAL(_url + "\n" + dn1 + " " + futuretime.str(Arc::MDSTime) + '\n', _readFile(meta_file));

  // expired DN
  Arc::Time pasttime = Arc::Time(now.GetTime() - 10);
  CPPUNIT_ASSERT(_createFile(meta_file, _url + "\n" + dn1 + " " + pasttime.str(Arc::MDSTime) + '\n'));
  CPPUNIT_ASSERT(!_fc1->CheckDN(_url, dn1));

  // add again
  futuretime = Arc::Time(now.GetTime() + 86400);
  CPPUNIT_ASSERT(_fc1->AddDN(_url, dn1, futuretime));
  CPPUNIT_ASSERT(_fc1->CheckDN(_url, dn1));
  CPPUNIT_ASSERT_EQUAL(_url + "\n" + dn1 + " " + futuretime.str(Arc::MDSTime) + '\n', _readFile(meta_file));

  // add another DN
  std::string dn2 = "/O=Grid/O=NorduGrid/OU=test.org/CN=Mrs Tester";
  CPPUNIT_ASSERT(!_fc1->CheckDN(_url, dn2));

  CPPUNIT_ASSERT(_fc1->AddDN(_url, dn2, futuretime));
  CPPUNIT_ASSERT(_fc1->CheckDN(_url, dn1));
  CPPUNIT_ASSERT(_fc1->CheckDN(_url, dn2));
  CPPUNIT_ASSERT_EQUAL(_url + "\n" + dn1 + " " + futuretime.str(Arc::MDSTime) + "\n" + dn2 + " " + futuretime.str(Arc::MDSTime) + '\n', _readFile(meta_file));

  // create expired DN and check it gets removed
  pasttime = Arc::Time(now.GetTime() - 86401);
  CPPUNIT_ASSERT(_createFile(meta_file, _url + '\n' + dn2 + " " + pasttime.str(Arc::MDSTime) + "\n" + dn1 + " " + pasttime.str(Arc::MDSTime) + '\n'));
  CPPUNIT_ASSERT(_fc1->AddDN(_url, dn1, futuretime));
  CPPUNIT_ASSERT(_fc1->CheckDN(_url, dn1));
  CPPUNIT_ASSERT_EQUAL(_url + "\n" + dn1 + " " + futuretime.str(Arc::MDSTime) + '\n', _readFile(meta_file));

  // add with no specified expiry time
  CPPUNIT_ASSERT(_fc1->AddDN(_url, dn2, Arc::Time(0)));
  // The following test fails occasionally for unknown reasons so is commented out
  // test should not fail if time changes during the test
  //CPPUNIT_ASSERT((_url + "\n" + dn1 + " " + futuretime.str(Arc::MDSTime) + "\n" + dn2 + " " + futuretime.str(Arc::MDSTime) + '\n') == _readFile(meta_file) ||
  //               (Arc::Time().GetTime() != now.GetTime()));

  // lock meta file - check should still work
  CPPUNIT_ASSERT(_createFile(meta_file + ".lock", std::string("1@" + _hostname)));
  CPPUNIT_ASSERT(!_fc1->AddDN(_url, dn1, futuretime));
  CPPUNIT_ASSERT(_fc1->CheckDN(_url, dn1));
}

void FileCacheTest::testTwoCaches() {

  // set up two caches
  std::vector<std::string> caches;
  caches.push_back(_cache_dir);
  std::string cache_dir2 = _cache_dir + "2";
  caches.push_back(cache_dir2);

  std::string url2 = "http://host.org/file2";

  Arc::FileCache *fc2 = new Arc::FileCache(caches, "1", _uid, _gid);
  // create cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(fc2->Start(_url, available, is_locked));
  std::string cache_file1 = fc2->File(_url);

  // test cache is ok
  CPPUNIT_ASSERT(*fc2);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  bool available2 = false;
  bool is_locked2 = false;
  CPPUNIT_ASSERT(fc2->Start(url2, available2, is_locked2));
  std::string cache_file2 = fc2->File(url2);

  // test cache is ok
  CPPUNIT_ASSERT(!available2);
  CPPUNIT_ASSERT(!is_locked2);

  // create cache files
  CPPUNIT_ASSERT(_createFile(cache_file1));
  CPPUNIT_ASSERT(_createFile(cache_file2));

  // create links
  std::string soft_link = _session_dir + "/" + _jobid + "/file1";
  std::string soft_link2 = _session_dir + "/" + _jobid + "/file2";
  
  // we expect the hard links to be made to here
  std::string hard_link_cache1 = _cache_job_dir + "/" + _jobid + "/file1";
  std::string hard_link_cache2 = cache_dir2 + "/joblinks/" + _jobid + "/file1";
  std::string hard_link2_cache1 = _cache_job_dir + "/" + _jobid + "/file2";
  std::string hard_link2_cache2 = cache_dir2 + "/joblinks/" + _jobid + "/file2";

  bool try_again = false;
  CPPUNIT_ASSERT(fc2->Link(soft_link, _url, false, false, true, try_again));
  CPPUNIT_ASSERT(fc2->Link(soft_link2, url2, false, false, true, try_again));

  // check hard links are made in one of the caches
  struct stat fileStat;
  CPPUNIT_ASSERT(stat(hard_link_cache1.c_str(), &fileStat) == 0 || stat(hard_link_cache2.c_str(), &fileStat) == 0);
  CPPUNIT_ASSERT(stat(hard_link2_cache1.c_str(), &fileStat) == 0 || stat(hard_link2_cache2.c_str(), &fileStat) == 0);

  // Stop caches to release locks
  CPPUNIT_ASSERT(fc2->Stop(_url));
  CPPUNIT_ASSERT(fc2->Stop(url2));

  // release with correct IDs
  CPPUNIT_ASSERT(fc2->Release());

  // check links and job dir are gone
  CPPUNIT_ASSERT(stat(hard_link_cache1.c_str(), &fileStat) != 0 && stat(hard_link_cache2.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT(stat(hard_link2_cache1.c_str(), &fileStat) != 0 && stat(hard_link2_cache2.c_str(), &fileStat) != 0);

  CPPUNIT_ASSERT(stat(std::string(_cache_job_dir + "/" + _jobid).c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT(stat(std::string(cache_dir2 + "/joblinks/" + _jobid).c_str(), &fileStat) != 0);

  // copy file
  CPPUNIT_ASSERT_EQUAL(0, remove(soft_link.c_str()));
  CPPUNIT_ASSERT(fc2->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(fc2->Link(soft_link, _url, true, false, false, try_again));

  // check job dir is created
  CPPUNIT_ASSERT(stat(hard_link_cache1.c_str(), &fileStat) == 0 || stat(hard_link_cache2.c_str(), &fileStat) == 0);

  CPPUNIT_ASSERT(fc2->Release());
}

void FileCacheTest::testCreationDate() {

  // call with non-existent file
  CPPUNIT_ASSERT(!_fc1->CheckCreated(_url));
  CPPUNIT_ASSERT_EQUAL(0, (int)(_fc1->GetCreated(_url).GetTime()));

  // Start cache and add file
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));

  // test creation date is available
  CPPUNIT_ASSERT(_fc1->CheckCreated(_url));

  // get creation date from file system
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Could not stat cache file " + _fc1->File(_url), 0, stat(_fc1->File(_url).c_str(), &fileStat));

  // test this is equal to created()
  CPPUNIT_ASSERT_EQUAL(fileStat.st_ctime, _fc1->GetCreated(_url).GetTime());

  // sleep 1 second and check dates still match
  sleep(1);
  CPPUNIT_ASSERT(fileStat.st_ctime == _fc1->GetCreated(_url).GetTime());

  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));

}

void FileCacheTest::testConstructor() {
  // constructor should not create anything
  struct stat fileStat;
  CPPUNIT_ASSERT(stat(_cache_data_dir.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT(stat(_cache_job_dir.c_str(), &fileStat) != 0);
  // create constructor with same parameters
  Arc::FileCache *fc2 = new Arc::FileCache(_cache_dir, _jobid, _uid, _gid);
  CPPUNIT_ASSERT(*_fc1 == *fc2);
  delete fc2;
  // test copy constructor
  Arc::FileCache *fc3 = new Arc::FileCache(*_fc1);
  CPPUNIT_ASSERT(*_fc1 == *fc3);
  delete fc3;
  // test invalid cache constructor, and that cache is not available
  Arc::FileCache *fc5 = new Arc::FileCache();
  CPPUNIT_ASSERT(!(*_fc1 == *fc5));
  CPPUNIT_ASSERT(!(*fc5));
  delete fc5;

  // create with 2 cache dirs
  std::vector<std::string> caches;
  std::string cache_dir2 = _cache_dir + "2";
  std::string cache_dir3 = _cache_dir + "3/";

  caches.push_back(_cache_dir);
  caches.push_back(cache_dir2);

  Arc::FileCache *fc6 = new Arc::FileCache(caches, _jobid, _uid, _gid);
  CPPUNIT_ASSERT(*fc6);
  CPPUNIT_ASSERT(!(*_fc1 == *fc6));

  // create with two different caches and compare
  caches.clear();
  caches.push_back(_cache_dir);
  caches.push_back(cache_dir3);
  Arc::FileCache *fc7 = new Arc::FileCache(caches, _jobid, _uid, _gid);
  CPPUNIT_ASSERT(*fc7);
  CPPUNIT_ASSERT(!(*fc6 == *fc7));
  delete fc6;
  delete fc7;

  // constructor with draining caches
  caches.clear();
  caches.push_back(_cache_dir);
  std::vector<std::string> draining_caches;
  draining_caches.push_back(_testroot + "draining1");

  Arc::FileCache *fc8 = new Arc::FileCache(caches, draining_caches, _jobid, _uid, _gid);
  CPPUNIT_ASSERT(*fc8);

  // file should be in main cache
  std::string hash = "/8a/929b8384300813ba1dd2d661c42835b80691a2";
  CPPUNIT_ASSERT_EQUAL(std::string(_cache_data_dir + hash), fc8->File(_url));
  delete fc8;
}

void FileCacheTest::testBadConstructor() {

  // no cache dir
  _fc1 = new Arc::FileCache("", _jobid, _uid, _gid);
  CPPUNIT_ASSERT(!(*_fc1));
  delete _fc1;
  // two caches, one of which is bad
  std::vector<std::string> caches;
  caches.push_back(_cache_dir);
  caches.push_back("");
  _fc1 = new Arc::FileCache(caches, _jobid, _uid, _gid);
  CPPUNIT_ASSERT(!(*_fc1));
  // call some methods
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(!(_fc1->Start(_url, available, is_locked)));
  CPPUNIT_ASSERT_EQUAL(std::string(""), _fc1->File(_url));
  CPPUNIT_ASSERT(!(_fc1->Stop(_url)));
  CPPUNIT_ASSERT(!(_fc1->StopAndDelete(_url)));
  CPPUNIT_ASSERT(!(_fc1->CheckCreated(_url)));

  caches.clear();
  caches.push_back(_cache_dir);

  // bad draining cache
  std::vector<std::string> draining_caches;
  draining_caches.push_back("");
  delete _fc1;
  _fc1 = new Arc::FileCache(caches, draining_caches, _jobid, _uid, _gid);
  CPPUNIT_ASSERT(!(*_fc1));
}

void FileCacheTest::testInternal() {

  // read a non-existent file
  std::string pid(Arc::tostring(getpid()));
  std::string testfile(_testroot + "/test.file." + pid);
  CPPUNIT_ASSERT(_readFile(testfile) == "");

  // create a file
  CPPUNIT_ASSERT(_createFile(testfile, pid));

  // check it exists
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL(0, stat(testfile.c_str(), &fileStat));

  // check the contents
  CPPUNIT_ASSERT_EQUAL(pid, _readFile(testfile));

  // delete
  CPPUNIT_ASSERT_EQUAL(0, remove((char*)testfile.c_str()));

  // check it has gone
  CPPUNIT_ASSERT(stat(testfile.c_str(), &fileStat) != 0);
}

bool FileCacheTest::_createFile(std::string filename, std::string text) {

  if (Arc::FileCreate(filename, text))
    return true;

  // try to create necessary dirs if possible
  if (filename.rfind('/') == 0 || filename.rfind('/') == std::string::npos)
    return false;
  Arc::DirCreate(filename.substr(0, filename.rfind('/')), 0700, true);
  return Arc::FileCreate(filename, text);
}

std::string FileCacheTest::_readFile(std::string filename) {

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

CPPUNIT_TEST_SUITE_REGISTRATION(FileCacheTest);
