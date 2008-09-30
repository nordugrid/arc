#include <cppunit/extensions/HelperMacros.h>

#include "../FileCache.h"

#include <cerrno>

#include <list>
#include <utime.h>
#include <sys/stat.h>
#include <sys/utsname.h>

class FileCacheTest : public CppUnit::TestFixture { 

  CPPUNIT_TEST_SUITE (FileCacheTest);
  CPPUNIT_TEST (testStart);
  CPPUNIT_TEST (testStop);
  CPPUNIT_TEST (testStopAndDelete);
  CPPUNIT_TEST (testLinkFile);
  CPPUNIT_TEST (testLinkFileLinkCache);
  CPPUNIT_TEST (testCopyFile);
  CPPUNIT_TEST (testFile);
  CPPUNIT_TEST (testRelease);
  CPPUNIT_TEST (testTwoCaches);
  CPPUNIT_TEST (testCreationDate);
  CPPUNIT_TEST (testValidityDate);
  CPPUNIT_TEST (testConstructor);
  CPPUNIT_TEST (testBadConstructor);
  CPPUNIT_TEST (testInternal);
  CPPUNIT_TEST_SUITE_END ();
  
public: 
  void setUp();
  void tearDown();
  
  void testStart();
  void testStop();
  void testStopAndDelete();
  void testLinkFile();
  void testLinkFileLinkCache();
  void testCopyFile();
  void testFile();
  void testRelease();
  void testTwoCaches();
  void testCreationDate();
  void testValidityDate();
  void testConstructor();
  void testBadConstructor();
  void testInternal();
  
private:
  std::string _cache_dir;
  std::string _cache_job_dir;
  std::string _session_dir;
  std::string _url;
  std::string _jobid;
  uid_t _uid;
  gid_t _gid;
  Arc::FileCache *_fc1;
  /** A list of files to be removed after each test */
  std::list<std::string> _files;
  /** Create a file with the given size */
  bool _createFile(std::string filename, std::string text = "a");
  /** Return the contents of the given file */
  std::string _readFile(std::string filename);
  /** Convert an int to string */
  std::string _intToString(int i);
};



void FileCacheTest::setUp() {

  _cache_dir = "/tmp/"+_intToString(getpid());
  _cache_job_dir = "/tmp/"+_intToString(getpid())+"_jobcache";
  _session_dir = "/tmp/"+_intToString(getpid())+"_session";
  
  // remove directories that may have been created
  rmdir(std::string(_cache_dir+"/69/59/db").c_str());
  rmdir(std::string(_cache_dir+"/69/59").c_str());
  rmdir(std::string(_cache_dir+"/69").c_str());
  rmdir(std::string(_session_dir+"/"+_jobid).c_str());
  rmdir(std::string(_session_dir).c_str());
  rmdir(std::string(_cache_job_dir+"/"+_jobid).c_str());
  rmdir(std::string(_cache_job_dir).c_str());
  rmdir(std::string(_cache_dir).c_str());

  _url = "rls://rls1.ndgf.org/file1";
  _uid = getuid();
  _gid = getgid();
  _jobid = "1";
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", _jobid, _uid, _gid, 2, 3);
}

void FileCacheTest::tearDown() {
  // delete all possible files that might have been created
  for (std::list<std::string>::iterator i = _files.begin(); i != _files.end(); ++i) {
    remove((char*)(*i).c_str());
  }
  _files.clear();

  // remove directories that have been created
  rmdir(std::string(_cache_dir+"/69/59/db").c_str());
  rmdir(std::string(_cache_dir+"/69/59").c_str());
  rmdir(std::string(_cache_dir+"/69").c_str());
  rmdir(std::string(_session_dir+"/"+_jobid).c_str());
  rmdir(std::string(_session_dir).c_str());
  rmdir(std::string(_cache_job_dir+"/"+_jobid).c_str());
  rmdir(std::string(_cache_job_dir).c_str());
  rmdir(std::string(_cache_dir).c_str());
 
  delete _fc1;
}
  
void FileCacheTest::testStart() {
  
  // test that the cache Starts ok
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  
  // test cache file is locked
  std::string lock_file(_fc1->File(_url) + ".lock");
  struct stat fileStat; 
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat lock file "+lock_file, 0, stat( lock_file.c_str(), &fileStat) );
  
  // test meta file exists and contains correct url
  std::string meta_file(_fc1->File(_url) + ".meta");
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat meta file "+meta_file, 0, stat( meta_file.c_str(), &fileStat) );
  std::string meta_url = _readFile(meta_file);
  CPPUNIT_ASSERT(meta_url != "");
  CPPUNIT_ASSERT_EQUAL(std::string(_url), meta_url);

  // test calling Start() again is ok
  // waits for timeout so takes long time
  //CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  //CPPUNIT_ASSERT(*_fc1);
  //CPPUNIT_ASSERT(!available);
  
  // look at modification time - should not be more than 1 second old
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat lock file "+lock_file, 0, stat( lock_file.c_str(), &fileStat) );
  time_t mod_time = fileStat.st_mtime;
  time_t now = time(NULL);
  CPPUNIT_ASSERT( (now - mod_time) <= 1 );
    
  // check it has the right pid inside
  std::string lock_pid = _readFile(lock_file);
  CPPUNIT_ASSERT(lock_pid != "");
  
  // construct hostname
  struct utsname buf;
  CPPUNIT_ASSERT_EQUAL(uname(&buf), 0);
  std::string host(buf.nodename);
  
  CPPUNIT_ASSERT_EQUAL(_intToString(getpid())+"@"+host, lock_pid);

  // set old modification time
  struct utimbuf times;
  time_t t = 1;
  times.actime = t;
  times.modtime = t;
  CPPUNIT_ASSERT_EQUAL(0, utime(lock_file.c_str(), &times));
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat lock file "+lock_file, 0, stat( lock_file.c_str(), &fileStat) );
  CPPUNIT_ASSERT_EQUAL(t, fileStat.st_mtime);
  
  // call Start again - should succeed and make new lock file
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // look at modification time - should not be more than 1 second old
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat lock file "+lock_file, 0, stat( lock_file.c_str(), &fileStat) );
  mod_time = fileStat.st_mtime;
  now = time(NULL);
  CPPUNIT_ASSERT( (now - mod_time) <= 1 );
 
  // create a cache file
  _createFile(_fc1->File(_url));
  
  // Stop cache
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  // check lock is released
  CPPUNIT_ASSERT( stat( lock_file.c_str(), &fileStat) != 0 ); 
  // check meta file is still there
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat meta file "+meta_file, 0, stat( meta_file.c_str(), &fileStat) );
  
  // call Start again and check available is true
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(available);
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  
  // lock the file with a pid which is still running on this host and check is_locked
  _createFile(_fc1->File(_url)+".lock", "1@" + host);
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(is_locked);
  
  // lock with process on different host
  is_locked = false;
  _createFile(_fc1->File(_url)+".lock", "1@mybadhost.org");
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(is_locked);
  
  // delete lock file and try again with a non-existant pid
  CPPUNIT_ASSERT_EQUAL(0, remove(std::string(_fc1->File(_url)+".lock").c_str()));
  _createFile(_fc1->File(_url)+".lock", "99999@" + host);
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // Stop cache
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  
  // put different url in meta file
  _createFile(_fc1->File(_url)+".meta", "http://badfile 1234567890");  
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
  _createFile(_fc1->File(_url)+".meta", "rls://rls1.ndgf.org/file1.bad");  
  CPPUNIT_ASSERT(!_fc1->Start(_url, available, is_locked));
  
}

void FileCacheTest::testStop() {
  
  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
  
  // Stop cache with non-existent cache file - this is ok
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  // check lock is released
  std::string lock_file(_fc1->File(_url) + ".lock");
  struct stat fileStat; 
  CPPUNIT_ASSERT( stat( lock_file.c_str(), &fileStat) != 0 ); 

  // Start again to create a new lock
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  
  // create the cache file
  _createFile(_fc1->File(_url));
  // Stop cache
  CPPUNIT_ASSERT(_fc1->Stop(_url));
  
  // check lock is released
  CPPUNIT_ASSERT( stat( lock_file.c_str(), &fileStat) != 0 );
  
  // check cache file is still there
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat cache file "+_fc1->File(_url), 0, stat( _fc1->File(_url).c_str(), &fileStat) );
  
  // check meta file is still there with correct url
  std::string meta_file(_fc1->File(_url) + ".meta");
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat meta file "+meta_file, 0, stat( meta_file.c_str(), &fileStat) );
  std::string meta_url = _readFile(meta_file);
  CPPUNIT_ASSERT(meta_url != "");
  CPPUNIT_ASSERT_EQUAL(std::string(_url), meta_url);

  // call with non-existent lock file
  CPPUNIT_ASSERT(!_fc1->Stop(_url));
  
  // put different pid in lock file - Stop() should return false
  _createFile(_fc1->File(_url)+".lock", _intToString(getpid()+1));
  CPPUNIT_ASSERT(!_fc1->Stop(_url));
  
  // check that lock and cache file are still there
  CPPUNIT_ASSERT( stat( _fc1->File(_url).c_str(), &fileStat) == 0 );
  CPPUNIT_ASSERT( stat( (_fc1->File(_url)+".lock").c_str(), &fileStat) == 0 );

  // check meta file is still there
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat meta file "+meta_file, 0, stat( meta_file.c_str(), &fileStat) );
}

void FileCacheTest::testStopAndDelete() {

  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // Stop and delete cache file with non-existent cache file
  CPPUNIT_ASSERT(_fc1->StopAndDelete(_url));
  // check lock is released
  std::string lock_file(_fc1->File(_url) + ".lock");
  struct stat fileStat; 
  CPPUNIT_ASSERT( stat( lock_file.c_str(), &fileStat) != 0 ); 

  // check meta file is deleted
  std::string meta_file(_fc1->File(_url) + ".meta");
  CPPUNIT_ASSERT( stat( meta_file.c_str(), &fileStat) != 0 );

  // Start again to create a new lock
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));

  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  
  // create the cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));
  // check cache file is there
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat cache file "+_fc1->File(_url), 0, stat( _fc1->File(_url).c_str(), &fileStat) );

  // Stop cache and delete file
  CPPUNIT_ASSERT(_fc1->StopAndDelete(_url));
  
  // check lock is released
  CPPUNIT_ASSERT( stat( lock_file.c_str(), &fileStat) != 0 );
  
  // check cache file has gone
  CPPUNIT_ASSERT( stat( _fc1->File(_url).c_str(), &fileStat) != 0 );

  // check meta file is deleted
  CPPUNIT_ASSERT( stat( meta_file.c_str(), &fileStat) != 0 );

  // create the cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));

  // call with non-existent lock file
  CPPUNIT_ASSERT(!_fc1->StopAndDelete(_url));
  
  // check the cache file is still there
  CPPUNIT_ASSERT( stat( _fc1->File(_url).c_str(), &fileStat) == 0 );
  
  // put different pid in lock file - StopAndDelete() should return false
  _createFile(_fc1->File(_url)+".lock", _intToString(getpid()+1));
  CPPUNIT_ASSERT(!_fc1->StopAndDelete(_url));
  
  // check that lock and cache files are still there
  CPPUNIT_ASSERT( stat( _fc1->File(_url).c_str(), &fileStat) == 0 );
  CPPUNIT_ASSERT( stat( (_fc1->File(_url)+".lock").c_str(), &fileStat) == 0 );

}

void FileCacheTest::testLinkFile() {
  
  std::string soft_link = _session_dir+"/"+_jobid+"/file1";
  std::string hard_link = _cache_job_dir+"/"+_jobid+"/file1";
  // link non-existant file
  CPPUNIT_ASSERT(!_fc1->Link(soft_link, _url));
  
  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
 
  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));
 
  // check cache file is there
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat cache file "+_fc1->File(_url), 0, stat( _fc1->File(_url).c_str(), &fileStat) );

  // create link
  CPPUNIT_ASSERT(_fc1->Link(soft_link, _url));
  _files.push_back(hard_link);
  _files.push_back(soft_link);
  
  // check hard- and soft-links exist
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat hard link "+hard_link, 0, stat( hard_link.c_str(), &fileStat) );
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat soft link "+soft_link, 0, stat( soft_link.c_str(), &fileStat) );

  // create bad soft-link
  if (_uid != 0 && stat("/lost+found/sessiondir", &fileStat) != 0 &&  errno == EACCES) 
    CPPUNIT_ASSERT(!_fc1->Link("/lost_found/sessiondir/file1", _url));
  
  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));
}

void FileCacheTest::testLinkFileLinkCache() {
  
  // new cache with link path set
  std::string cache_link_dir = "/tmp/"+_intToString(getpid())+"_link";
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, cache_link_dir, _jobid, _uid, _gid, 2, 3);
  CPPUNIT_ASSERT(symlink(_cache_job_dir.c_str(), cache_link_dir.c_str()) == 0);
  _files.push_back(cache_link_dir);
  _files.push_back(_fc1->File(_url)+".lock");
  
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
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat cache file "+_fc1->File(_url), 0, stat( _fc1->File(_url).c_str(), &fileStat) );

  // create link
  std::string soft_link = _session_dir+"/"+_jobid+"/file1";
  std::string hard_link = _cache_job_dir+"/"+_jobid+"/file1";
  CPPUNIT_ASSERT(_fc1->Link(soft_link, _url));
  _files.push_back(soft_link);
  _files.push_back(hard_link);
 
  // check soft link is ok and points to the right place
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat soft link "+soft_link, 0, lstat( soft_link.c_str(), &fileStat) );
  // check our soft link links to yet another link
  CPPUNIT_ASSERT(S_ISLNK(fileStat.st_mode));
  // check the target is correct
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat target of soft link "+soft_link, 0, stat( soft_link.c_str(), &fileStat) );

  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));
}

void FileCacheTest::testCopyFile() {
  
  std::string dest_file = _session_dir+"/"+_jobid+"/file1";

  // copy non-existant file
  CPPUNIT_ASSERT(!_fc1->Copy(dest_file, _url));
  
  // Start cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);

  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));
 
  // check cache file is there
  struct stat fileStat;
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat cache file "+_fc1->File(_url), 0, stat( _fc1->File(_url).c_str(), &fileStat) );

  // do copy
  CPPUNIT_ASSERT(_fc1->Copy(dest_file, _url));
  _files.push_back(dest_file);
  
  // check copy exists
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat destination file "+dest_file, 0, stat( dest_file.c_str(), &fileStat) );

  // create bad copy
  if (_uid != 0 && stat("/lost+found/sessiondir", &fileStat) != 0 &&  errno == EACCES)
    CPPUNIT_ASSERT(!_fc1->Copy("/lost_found/sessiondir/file1", _url));
  
  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));
}

void FileCacheTest::testFile() {
  // test hash returned
  CPPUNIT_ASSERT_EQUAL( std::string(_cache_dir+"/69/59/db/aef4f0a0d9aa84368e01a35a78abf267ac"), _fc1->File(_url));
  // test no extra slash
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", "1", _uid, _gid, 2, 3);
  CPPUNIT_ASSERT_EQUAL( std::string(_cache_dir+"/69/59/db/aef4f0a0d9aa84368e01a35a78abf267ac"), _fc1->File(_url));
  // different dir structure
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", "1", _uid, _gid, 1, 2);
  CPPUNIT_ASSERT_EQUAL( std::string(_cache_dir+"/6/9/59dbaef4f0a0d9aa84368e01a35a78abf267ac"), _fc1->File(_url));
  // different dir structure
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", "1", _uid, _gid, 10, 3);
  CPPUNIT_ASSERT_EQUAL( std::string(_cache_dir+"/6959dbaef4/f0a0d9aa84/368e01a35a/78abf267ac"), _fc1->File(_url));
  // different dir structure
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", "1", _uid, _gid, 2, 19);
  CPPUNIT_ASSERT_EQUAL( std::string(_cache_dir+"/69/59/db/ae/f4/f0/a0/d9/aa/84/36/8e/01/a3/5a/78/ab/f2/67/ac"), _fc1->File(_url));
  
  // set up two caches
  std::vector<struct Arc::CacheParameters> caches;
  struct Arc::CacheParameters params;
  params.cache_path = _cache_dir;
  params.cache_job_dir_path = _cache_job_dir;
  params.cache_link_path = "/tmp/"+_intToString(getpid())+"_link";
  struct Arc::CacheParameters params2;
  params2.cache_path = "/tmp/"+_intToString(getpid())+"_cache2";
  params2.cache_job_dir_path = "/tmp/"+_intToString(getpid())+"_jobcache2";
  params2.cache_link_path = "";
  caches.push_back(params);
  caches.push_back(params2);

  _files.push_back(params2.cache_path);
  _files.push_back(params2.cache_job_dir_path+"/1");
  _files.push_back(params2.cache_job_dir_path);
  
  Arc::FileCache * fc2 = new Arc::FileCache(caches, "1", _uid, _gid, 2, 3);
  // _url should go to the first cache
  CPPUNIT_ASSERT_EQUAL( std::string(params.cache_path+"/69/59/db/aef4f0a0d9aa84368e01a35a78abf267ac"), fc2->File(_url));
  // this url goes to the second cache
  CPPUNIT_ASSERT_EQUAL( std::string(params2.cache_path+"/11/8f/1a/a74364c6546cfe2c536c8979bfd1609a7b"), fc2->File("rls://rls1.ndgf.org/file2"));
  delete fc2;
}

void FileCacheTest::testRelease() {
  
  // create cache and link
  std::string soft_link = _session_dir+"/"+_jobid+"/file1";
  std::string hard_link = _cache_job_dir+"/"+_jobid+"/file1";
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
 
  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));
 
  // create link
  CPPUNIT_ASSERT(_fc1->Link(soft_link, _url));
  _files.push_back(hard_link);
  _files.push_back(soft_link);
  
  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));

  // release
  CPPUNIT_ASSERT(_fc1->Release());
  
  // check files and dir are gone
  struct stat fileStat;
  CPPUNIT_ASSERT( stat(hard_link.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT( stat(std::string(_cache_job_dir+"/"+_jobid).c_str(), &fileStat) != 0);

  // copy file
  CPPUNIT_ASSERT_EQUAL(0, remove(soft_link.c_str()));
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(_fc1->Copy(soft_link, _url));
  
  // check job dir is not created
  CPPUNIT_ASSERT( stat(std::string(_cache_job_dir+"/"+_jobid).c_str(), &fileStat) != 0);

  // release should not give an error, even though job dir does not exist
  CPPUNIT_ASSERT(_fc1->Release());
}

void FileCacheTest::testTwoCaches() {
  
  // set up two caches
  std::vector<struct Arc::CacheParameters> caches;
  struct Arc::CacheParameters params;
  params.cache_path = _cache_dir;
  params.cache_job_dir_path = _cache_job_dir;
  params.cache_link_path = "/tmp/"+_intToString(getpid())+"_link";
  struct Arc::CacheParameters params2;
  params2.cache_path = _cache_dir+"_cache2";
  params2.cache_job_dir_path = _cache_job_dir+"_jobcache2";
  params2.cache_link_path = "";
  caches.push_back(params);
  caches.push_back(params2);
  std::string url2 = "rls://rls1.ndgf.org/file2";
  
  Arc::FileCache * fc2 = new Arc::FileCache(caches, "1", _uid, _gid, 2, 3);
  // create cache
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(fc2->Start(_url, available, is_locked)); 
  _files.push_back(fc2->File(_url)+".meta");
  _files.push_back(fc2->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*fc2);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
 
  bool available2 = false;
  bool is_locked2 = false;
  CPPUNIT_ASSERT(fc2->Start(url2, available2, is_locked2)); 
  _files.push_back(fc2->File(url2)+".meta");
  _files.push_back(fc2->File(url2)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(!available2);
  CPPUNIT_ASSERT(!is_locked2);
  
  // create cache files
  CPPUNIT_ASSERT(_createFile(fc2->File(_url)));
  CPPUNIT_ASSERT(_createFile(fc2->File(url2)));

  // create links
  std::string soft_link = _session_dir+"/"+_jobid+"/file1";
  std::string soft_link2 = _session_dir+"/"+_jobid+"/file2";
  // we expect the hard links to be made to here
  std::string hard_link = _cache_job_dir+"/"+_jobid+"/file1";
  std::string hard_link2 = params2.cache_job_dir_path+"/"+_jobid+"/file2";
  CPPUNIT_ASSERT(fc2->Link(soft_link, _url));
  CPPUNIT_ASSERT(fc2->Link(soft_link2, url2));
  _files.push_back(hard_link);
  _files.push_back(hard_link2);
  _files.push_back(soft_link);
  _files.push_back(soft_link2);
  _files.push_back(params2.cache_path+"/11/8f/1a");
  _files.push_back(params2.cache_path+"/11/8f");
  _files.push_back(params2.cache_path+"/11");
  _files.push_back(params2.cache_path);
  _files.push_back(params2.cache_job_dir_path+"/1");
  _files.push_back(params2.cache_job_dir_path);
  
  // check correct hard links are made
  struct stat fileStat;
  CPPUNIT_ASSERT( stat(hard_link.c_str(), &fileStat) == 0);
  CPPUNIT_ASSERT( stat(hard_link2.c_str(), &fileStat) == 0);
  
  // Stop caches to release locks
  CPPUNIT_ASSERT(fc2->Stop(_url));
  CPPUNIT_ASSERT(fc2->Stop(url2));

  // release with correct IDs
  CPPUNIT_ASSERT(fc2->Release());
  
  // check files and dir are gone
  CPPUNIT_ASSERT( stat(hard_link.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT( stat(hard_link2.c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT( stat(std::string(_cache_job_dir+"/"+_jobid).c_str(), &fileStat) != 0);
  CPPUNIT_ASSERT( stat(std::string(params2.cache_job_dir_path+"/"+_jobid).c_str(), &fileStat) != 0);

  // copy file
  CPPUNIT_ASSERT_EQUAL(0, remove(soft_link.c_str()));
  CPPUNIT_ASSERT(fc2->Start(_url, available, is_locked));
  CPPUNIT_ASSERT(fc2->Copy(soft_link, _url));
  
  // check job dir is not created
  CPPUNIT_ASSERT( stat(std::string(_cache_job_dir+"/"+_jobid).c_str(), &fileStat) != 0);

  // release should not give an error, even though job dir does not exist
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
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
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
  CPPUNIT_ASSERT_EQUAL_MESSAGE( "Could not stat cache file "+_fc1->File(_url), 0, stat( _fc1->File(_url).c_str(), &fileStat) );

  // test this is equal to created()
  CPPUNIT_ASSERT_EQUAL(fileStat.st_ctime, _fc1->GetCreated(_url).GetTime());
  
  // sleep 1 second and check dates still match
  sleep(1);
  CPPUNIT_ASSERT(fileStat.st_ctime == _fc1->GetCreated(_url).GetTime());
  
  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));

}

void FileCacheTest::testValidityDate() {
  
  // call with non-existent file
  CPPUNIT_ASSERT(!_fc1->CheckValid(_url));
  CPPUNIT_ASSERT_EQUAL(0, (int)(_fc1->GetValid(_url).GetTime()));
  CPPUNIT_ASSERT(!_fc1->SetValid(_url, Arc::Time(time(NULL))));
 
  // Start cache and add file
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT(_fc1->Start(_url, available, is_locked));
  _files.push_back(_fc1->File(_url)+".meta");
  _files.push_back(_fc1->File(_url)+".lock");
  
  // test cache is ok
  CPPUNIT_ASSERT(*_fc1);
  CPPUNIT_ASSERT(!available);
  CPPUNIT_ASSERT(!is_locked);
 
  // create cache file
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)));
 
  // test validity date is not available
  CPPUNIT_ASSERT(!_fc1->CheckValid(_url));
  
  // look inside the meta file to check
  std::string meta_file = _fc1->File(_url)+".meta";
  CPPUNIT_ASSERT_EQUAL(_url, _readFile(meta_file));
  
  // set validity time to now
  Arc::Time now;
  CPPUNIT_ASSERT(_fc1->SetValid(_url, now));
  CPPUNIT_ASSERT(_fc1->CheckValid(_url));
  CPPUNIT_ASSERT_EQUAL(now, _fc1->GetValid(_url));
  CPPUNIT_ASSERT_EQUAL(_url+" "+now.str(Arc::MDSTime), _readFile(meta_file));
  
  // put bad format inside metafile
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)+".meta", _url+" abcd"));
  CPPUNIT_ASSERT_EQUAL(0, (int)(_fc1->GetValid(_url).GetTime()));
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)+".meta", _url+"abc 1234567890"));
  CPPUNIT_ASSERT_EQUAL(0, (int)(_fc1->GetValid(_url).GetTime()));
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)+".meta", _url+"abc1234567890"));
  CPPUNIT_ASSERT_EQUAL(0, (int)(_fc1->GetValid(_url).GetTime()));
  // cannot be more than MAX_INT
  CPPUNIT_ASSERT(_createFile(_fc1->File(_url)+".meta", _url+" 1234567890123"));
  CPPUNIT_ASSERT_EQUAL(0, (int)(_fc1->GetValid(_url).GetTime()));  

  // set new time
  Arc::Time newtime(time(NULL)+10);
  CPPUNIT_ASSERT(_fc1->SetValid(_url, newtime));
  CPPUNIT_ASSERT_EQUAL(newtime, _fc1->GetValid(_url)); 
  CPPUNIT_ASSERT_EQUAL(_url+" "+newtime.str(Arc::MDSTime), _readFile(meta_file));
  
  // Stop cache to release lock
  CPPUNIT_ASSERT(_fc1->Stop(_url));

}

void FileCacheTest::testConstructor() {
  // permissions testing of dirs created by the constructor
  struct stat fileStat;
  CPPUNIT_ASSERT( stat(_cache_dir.c_str(), &fileStat) == 0);
  CPPUNIT_ASSERT((fileStat.st_mode & (S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH)) == (S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH));
  CPPUNIT_ASSERT( stat(_cache_job_dir.c_str(), &fileStat) == 0);
  CPPUNIT_ASSERT((fileStat.st_mode & (S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH)) == (S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH));
  // create constructor with same parameters
  Arc::FileCache *fc2 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", _jobid, _uid, _gid, 2, 3);
  CPPUNIT_ASSERT( *_fc1 == *fc2 );
  delete fc2;
  // test copy constructor
  Arc::FileCache *fc3 = new Arc::FileCache(*_fc1);
  CPPUNIT_ASSERT( *_fc1 == *fc3 );
  delete fc3;
  // create with different parameters
  Arc::FileCache *fc4 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", _jobid, _uid, _gid, 2, 4);
  CPPUNIT_ASSERT( !(*_fc1 == *fc4) );
  delete fc4;
  // test invalid cache constructor, and that cache is not available
  Arc::FileCache *fc5 = new Arc::FileCache();
  CPPUNIT_ASSERT( !(*_fc1 == *fc5) );
  CPPUNIT_ASSERT( !(*fc5) );
  delete fc5;
  
  // create with 2 cache dirs
  std::vector<struct Arc::CacheParameters> caches;
  struct Arc::CacheParameters params;
  params.cache_path = _cache_dir;
  params.cache_job_dir_path = _cache_job_dir;
  params.cache_link_path = "/tmp/"+_intToString(getpid())+"_link";
  struct Arc::CacheParameters params2;
  params2.cache_path = "/tmp/"+_intToString(getpid())+"_cache2";
  params2.cache_job_dir_path = "/tmp/"+_intToString(getpid())+"_jobcache2";
  params2.cache_link_path = "";
  struct Arc::CacheParameters params3;
  params3.cache_path = "/tmp/"+_intToString(getpid())+"_cache3";
  params3.cache_job_dir_path = "/tmp/"+_intToString(getpid())+"_jobcache3";
  params3.cache_link_path = "";
  
  _files.push_back(params2.cache_path);
  _files.push_back(params2.cache_job_dir_path+"/1");
  _files.push_back(params2.cache_job_dir_path);
  _files.push_back(params3.cache_path);
  _files.push_back(params3.cache_job_dir_path+"/1");
  _files.push_back(params3.cache_job_dir_path);
  
  caches.push_back(params);
  caches.push_back(params2);
  
  Arc::FileCache *fc6 = new Arc::FileCache(caches, _jobid, _uid, _gid, 2, 3);
  CPPUNIT_ASSERT(*fc6);
  CPPUNIT_ASSERT( !(*_fc1 == *fc6) );
  
  // create with two different caches and compare
  caches.empty();
  caches.push_back(params);
  caches.push_back(params3);
  Arc::FileCache *fc7 = new Arc::FileCache(caches, _jobid, _uid, _gid, 2, 3);
  CPPUNIT_ASSERT(*fc7);
  CPPUNIT_ASSERT( !(*fc6 == *fc7) );
  
  delete fc6;
  delete fc7;
}

void FileCacheTest::testBadConstructor() {
  delete _fc1;
  // dir length too long
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", _jobid, _uid, _gid, 20, 3);
  CPPUNIT_ASSERT( !(*_fc1) );
  delete _fc1;
  // too many dirs
  _fc1 = new Arc::FileCache(_cache_dir, _cache_job_dir, "", _jobid, _uid, _gid, 3, 20);
  CPPUNIT_ASSERT( !(*_fc1) );
  delete _fc1;
  // job cache inside cache
  _fc1 = new Arc::FileCache(_cache_dir, _cache_dir+"/jobcache", "", _jobid, _uid, _gid, 3, 2);
  CPPUNIT_ASSERT( !(*_fc1) );
  delete _fc1;
  // permission denied
  struct stat fileStat;
  if (_uid != 0 && stat("/lost+found/cache", &fileStat) != 0 &&  errno == EACCES) {
    _fc1 = new Arc::FileCache("/lost+found/cache", _cache_job_dir, "", _jobid, _uid, _gid, 3, 2);
    CPPUNIT_ASSERT( !(*_fc1) );
    delete _fc1;
  }
  // no cache dir
  _fc1 = new Arc::FileCache("", _cache_job_dir, "", _jobid, _uid, _gid, 2, 3);
  CPPUNIT_ASSERT( !(*_fc1) );
  delete _fc1;
  // two caches, one of which is bad
  std::vector<struct Arc::CacheParameters> caches;
  struct Arc::CacheParameters params;
  params.cache_path = _cache_dir;
  params.cache_job_dir_path = _cache_job_dir;
  params.cache_link_path = "/tmp/"+_intToString(getpid())+"_link";
  struct Arc::CacheParameters params2;
  params2.cache_path = "";
  params2.cache_job_dir_path = "/tmp/"+_intToString(getpid())+"_jobcache2";
  params2.cache_link_path = "";
  caches.push_back(params);
  caches.push_back(params2);
  _fc1 = new Arc::FileCache(caches, _jobid, _uid, _gid, 2, 3);
  CPPUNIT_ASSERT( !(*_fc1) );
  // call some methods
  bool available = false;
  bool is_locked = false;
  CPPUNIT_ASSERT( !(_fc1->Start(_url, available, is_locked)) );
  CPPUNIT_ASSERT_EQUAL(std::string(""), _fc1->File(_url));
  CPPUNIT_ASSERT( !(_fc1->Stop(_url)));
  CPPUNIT_ASSERT( !(_fc1->StopAndDelete(_url)));
  CPPUNIT_ASSERT( !(_fc1->CheckCreated(_url)));
}

void FileCacheTest::testInternal() {
  
  // read a non-existant file
  std::string pid(_intToString(getpid()));
  std::string testfile("test.file." + pid);
  CPPUNIT_ASSERT(_readFile(testfile) == "");
  
  // create a file
  CPPUNIT_ASSERT(_createFile(testfile, pid));
  
  // check it exists
  struct stat fileStat; 
  CPPUNIT_ASSERT_EQUAL(0, stat( testfile.c_str(), &fileStat)); 

  // check the contents
  CPPUNIT_ASSERT_EQUAL(pid, _readFile(testfile));
  
  // delete
  CPPUNIT_ASSERT_EQUAL(0, remove((char*)testfile.c_str()));
  
  // check it has gone
  CPPUNIT_ASSERT(stat( testfile.c_str(), &fileStat) != 0); 
}

bool FileCacheTest::_createFile(std::string filename, std::string text) {
  
  FILE * pFile;
  pFile = fopen ((char*)filename.c_str(), "w");
  if (pFile == NULL) return false;
  fputs ((char*)text.c_str(), pFile);
  fclose (pFile);
  _files.push_back(filename);
  return true;
}

std::string FileCacheTest::_readFile(std::string filename) {
  
  FILE * pFile;
  char mystring [1024]; // should be long enough for a pid or url...
  pFile = fopen ((char*)filename.c_str(), "r");
  if (pFile == NULL) return "";
  fgets (mystring, sizeof(mystring), pFile);
  fclose (pFile);
  
  return std::string(mystring);  
}

std::string FileCacheTest::_intToString(int i) {
  std::stringstream out;
  out << i;
  return out.str();
}

CPPUNIT_TEST_SUITE_REGISTRATION (FileCacheTest);
