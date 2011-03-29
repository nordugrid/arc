#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/FileUtils.h>
#include <arc/FileAccess.h>

class FileAccessTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileAccessTest);
  CPPUNIT_TEST(TestOpenWriteReadStat);
  CPPUNIT_TEST(TestCopy);
  CPPUNIT_TEST(TestDir);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestOpenWriteReadStat();
  void TestCopy();
  void TestDir();

private:
  uid_t uid;
  gid_t gid;
  std::string testroot;
};


void FileAccessTest::setUp() {
  CPPUNIT_ASSERT(Arc::TmpDirCreate(testroot));
  CPPUNIT_ASSERT(!testroot.empty());
  if(getuid() == 0) {
    struct passwd* pwd = getpwnam("nobody");
    CPPUNIT_ASSERT(pwd);
    uid = pwd->pw_uid;
    gid = pwd->pw_gid;
    CPPUNIT_ASSERT_EQUAL(0,::chmod(testroot.c_str(),0777));
  } else {
    uid = getuid();
    gid = getgid();
  }
  Arc::FileAccess::testtune();
}

void FileAccessTest::tearDown() {
  Arc::DirDelete(testroot);
}

void FileAccessTest::TestOpenWriteReadStat() {
  Arc::FileAccess fa;
  std::string testfile = testroot+"/file1";
  std::string testdata = "test";
  CPPUNIT_ASSERT(fa.setuid(uid,gid));
  CPPUNIT_ASSERT(fa.open(testfile,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.write(testdata.c_str(),testdata.length()));
  CPPUNIT_ASSERT(fa.close());
  struct stat st;
  CPPUNIT_ASSERT_EQUAL(0,::stat(testfile.c_str(),&st));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)st.st_size);
  CPPUNIT_ASSERT_EQUAL(uid,st.st_uid);
  CPPUNIT_ASSERT_EQUAL(gid,st.st_gid);
  CPPUNIT_ASSERT_EQUAL(0600,(int)(st.st_mode & 0777));
  CPPUNIT_ASSERT(fa.open(testfile,O_RDONLY,0));
  char buf[16];
  struct stat st2;
  CPPUNIT_ASSERT(fa.fstat(st2));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.read(buf,sizeof(buf)));
  CPPUNIT_ASSERT(fa.close());
  std::string testdata2(buf,testdata.length());
  CPPUNIT_ASSERT_EQUAL(testdata,testdata2);
  CPPUNIT_ASSERT(::memcmp(&st,&st2,sizeof(struct stat)) == 0);
  CPPUNIT_ASSERT(fa.stat(testfile,st2));
  CPPUNIT_ASSERT(::memcmp(&st,&st2,sizeof(struct stat)) == 0);
}

void FileAccessTest::TestCopy() {
  Arc::FileAccess fa;
  std::string testfile1 = testroot+"/copyfile1";
  std::string testfile2 = testroot+"/copyfile2";
  std::string testdata = "copytest";
  CPPUNIT_ASSERT(fa.setuid(uid,gid));
  CPPUNIT_ASSERT(fa.open(testfile1,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.write(testdata.c_str(),testdata.length()));
  CPPUNIT_ASSERT(fa.close());
  CPPUNIT_ASSERT(fa.copy(testfile1,testfile2,0600));
  CPPUNIT_ASSERT(fa.open(testfile2,O_RDONLY,0));
  char buf[16];
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.read(buf,sizeof(buf)));
  CPPUNIT_ASSERT(fa.close());
  std::string testdata2(buf,testdata.length());
  CPPUNIT_ASSERT_EQUAL(testdata,testdata2);
}

void FileAccessTest::TestDir() {
  std::string testdir1 = testroot + "/dir1/dir2/dir3";
  std::string testdir2 = testroot + "/dir1/dir2/dir3/dir4";
  std::string testdir3 = testroot + "/dir1";
  Arc::FileAccess fa;
  CPPUNIT_ASSERT(fa.setuid(uid,gid));
  CPPUNIT_ASSERT(!fa.mkdir(testdir1,0700));
  CPPUNIT_ASSERT(fa.mkdirp(testdir1,0700));
  CPPUNIT_ASSERT(fa.mkdir(testdir2,0700));
  CPPUNIT_ASSERT(fa.opendir(testdir1));
  std::string name;
  while(true) {
    CPPUNIT_ASSERT(fa.readdir(name));
    if(name == ".") continue;
    if(name == "..") continue;
    break;
  }
  CPPUNIT_ASSERT(fa.closedir());
  CPPUNIT_ASSERT_EQUAL(testdir2.substr(testdir1.length()+1),name);
  CPPUNIT_ASSERT(!fa.rmdir(testdir3));
  CPPUNIT_ASSERT(fa.rmdir(testdir2));
  CPPUNIT_ASSERT(fa.rmdirr(testdir3));
}

/*
void FileAccessTest::TestFileLink() {
  CPPUNIT_ASSERT(_createFile(testroot + "/file1"));
  CPPUNIT_ASSERT(Arc::FileLink(testroot+"/file1", testroot+"/file1s", true));
  CPPUNIT_ASSERT(Arc::FileLink(testroot+"/file1", testroot+"/file1h", false));
  struct stat st;
  CPPUNIT_ASSERT(Arc::FileStat(testroot+"/file1s", &st, true));
  CPPUNIT_ASSERT_EQUAL(1, (int)st.st_size);
  CPPUNIT_ASSERT(Arc::FileStat(testroot+"/file1h", &st, true));
  CPPUNIT_ASSERT_EQUAL(1, (int)st.st_size);
  CPPUNIT_ASSERT_EQUAL(testroot+"/file1", Arc::FileReadLink(testroot+"/file1s"));
}

void FileAccessTest::TestFileCreateAndRead() {
  // create empty file
  std::string filename(testroot + "/file1");
  CPPUNIT_ASSERT(Arc::FileCreate(filename, ""));

  struct stat st;
  CPPUNIT_ASSERT(Arc::FileStat(filename, &st, true));
  CPPUNIT_ASSERT_EQUAL(0, (int)st.st_size);

  std::list<std::string> data;
  CPPUNIT_ASSERT(Arc::FileRead(filename, data));
  CPPUNIT_ASSERT(data.empty());

  // create again with some data
  CPPUNIT_ASSERT(Arc::FileCreate(filename, "12\nabc\n\nxyz\n"));

  CPPUNIT_ASSERT(Arc::FileRead(filename, data));
  CPPUNIT_ASSERT_EQUAL(4, (int)data.size());
  CPPUNIT_ASSERT_EQUAL(std::string("12"), data.front());
  CPPUNIT_ASSERT_EQUAL(std::string("xyz"), data.back());

  // remove file and check failure
  CPPUNIT_ASSERT_EQUAL(true, Arc::FileDelete(filename.c_str()));
  CPPUNIT_ASSERT(!Arc::FileRead(filename, data));
}

void FileAccessTest::TestDirOpen() {
  CPPUNIT_ASSERT(_createFile(testroot + "/file1"));
  CPPUNIT_ASSERT(_createFile(testroot + "/file2"));
  Glib::Dir* dir = Arc::DirOpen(testroot);
  std::list<std::string> entries (dir->begin(), dir->end());
  CPPUNIT_ASSERT_EQUAL(2, (int)entries.size());
  dir->rewind();
  std::string name;
  name = dir->read_name();
  CPPUNIT_ASSERT((name == "file1") || (name == "file2"));
  name = dir->read_name();
  CPPUNIT_ASSERT((name == "file1") || (name == "file2"));
  delete dir;
}

void FileAccessTest::TestMakeAndDeleteDir() {
  // create a few subdirs and files then recursively delete
  struct stat st;
  CPPUNIT_ASSERT(stat(testroot.c_str(), &st) == 0);
  CPPUNIT_ASSERT(_createFile(testroot + "/file1"));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot + "/dir1"), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(stat(std::string(testroot + "/dir1").c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(_createFile(testroot + "/dir1/file2"));
  // should fail if with_parents is set to false
  CPPUNIT_ASSERT(!Arc::DirCreate(std::string(testroot + "/dir1/dir2/dir3"), S_IRUSR | S_IWUSR | S_IXUSR, false));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot + "/dir1/dir2/dir3"), S_IRUSR | S_IWUSR | S_IXUSR, true));
  CPPUNIT_ASSERT(stat(std::string(testroot + "/dir1/dir2/dir3").c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(_createFile(testroot + "/dir1/dir2/dir3/file4"));
  CPPUNIT_ASSERT(symlink(std::string(testroot + "/dir1/dir2").c_str(), std::string(testroot + "/dir1/dir2/link1").c_str()) == 0);

  CPPUNIT_ASSERT(Arc::DirDelete(testroot));
  CPPUNIT_ASSERT(stat(testroot.c_str(), &st) != 0);
  
}

void FileAccessTest::TestTmpDirCreate() {
  std::string path;
  CPPUNIT_ASSERT(Arc::TmpDirCreate(path));
  struct stat st;
  CPPUNIT_ASSERT(stat(path.c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(Arc::DirDelete(path));
  CPPUNIT_ASSERT(stat(path.c_str(), &st) != 0);
}

bool FileAccessTest::_createFile(const std::string& filename, const std::string& text) {

  FILE *pFile;
  pFile = fopen((char*)filename.c_str(), "w");
  if (pFile == NULL) return false;
  fputs((char*)text.c_str(), pFile);
  fclose(pFile);
  return true;
}

*/

CPPUNIT_TEST_SUITE_REGISTRATION(FileAccessTest);
