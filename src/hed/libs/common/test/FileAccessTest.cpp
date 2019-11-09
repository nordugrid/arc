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
  CPPUNIT_TEST(TestRename);
  CPPUNIT_TEST(TestDir);
  CPPUNIT_TEST(TestSeekAllocate);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestOpenWriteReadStat();
  void TestCopy();
  void TestRename();
  void TestDir();
  void TestSeekAllocate();

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
  CPPUNIT_ASSERT(fa.fa_setuid(uid,gid));
  CPPUNIT_ASSERT(fa.fa_open(testfile,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.fa_write(testdata.c_str(),testdata.length()));
  CPPUNIT_ASSERT(fa.fa_close());
  struct stat st;
  CPPUNIT_ASSERT_EQUAL(0,::stat(testfile.c_str(),&st));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)st.st_size);
  CPPUNIT_ASSERT_EQUAL((int)uid,(int)st.st_uid);
  // Group ownership of a file is not guaranteed to be gid of user proces.
  // This is especially true on MAC OSX:
  //  https://bugzilla.nordugrid.org/show_bug.cgi?id=2089#c3
  //CPPUNIT_ASSERT_EQUAL(gid,st.st_gid);
  CPPUNIT_ASSERT_EQUAL(0600,(int)(st.st_mode & 0777));
  CPPUNIT_ASSERT(fa.fa_open(testfile,O_RDONLY,0));
  char buf[16];
  struct stat st2;
  CPPUNIT_ASSERT(fa.fa_fstat(st2));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.fa_read(buf,sizeof(buf)));
  CPPUNIT_ASSERT(fa.fa_close());
  std::string testdata2(buf,testdata.length());
  CPPUNIT_ASSERT_EQUAL(testdata,testdata2);
  CPPUNIT_ASSERT_EQUAL(st.st_mode,st2.st_mode);
  CPPUNIT_ASSERT_EQUAL(st.st_uid,st2.st_uid);
  CPPUNIT_ASSERT_EQUAL(st.st_gid,st2.st_gid);
  CPPUNIT_ASSERT_EQUAL(st.st_size,st2.st_size);
  CPPUNIT_ASSERT(fa.fa_stat(testfile,st2));
  CPPUNIT_ASSERT_EQUAL(st.st_mode,st2.st_mode);
  CPPUNIT_ASSERT_EQUAL(st.st_uid,st2.st_uid);
  CPPUNIT_ASSERT_EQUAL(st.st_gid,st2.st_gid);
  CPPUNIT_ASSERT_EQUAL(st.st_size,st2.st_size);
}

void FileAccessTest::TestCopy() {
  Arc::FileAccess fa;
  std::string testfile1 = testroot+"/copyfile1";
  std::string testfile2 = testroot+"/copyfile2";
  std::string testdata = "copytest";
  CPPUNIT_ASSERT(fa.fa_setuid(uid,gid));
  CPPUNIT_ASSERT(fa.fa_open(testfile1,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.fa_write(testdata.c_str(),testdata.length()));
  CPPUNIT_ASSERT(fa.fa_close());
  CPPUNIT_ASSERT(fa.fa_copy(testfile1,testfile2,0600));
  CPPUNIT_ASSERT(fa.fa_open(testfile2,O_RDONLY,0));
  char buf[16];
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.fa_read(buf,sizeof(buf)));
  CPPUNIT_ASSERT(fa.fa_close());
  std::string testdata2(buf,testdata.length());
  CPPUNIT_ASSERT_EQUAL(testdata,testdata2);
}

void FileAccessTest::TestRename() {
  Arc::FileAccess fa;
  std::string oldfile = testroot+"/oldfile";
  std::string newfile = testroot+"/newfile";
  std::string testdata = "renametest";
  CPPUNIT_ASSERT(fa.fa_setuid(uid,gid));
  CPPUNIT_ASSERT(fa.fa_open(oldfile,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)testdata.length(),(int)fa.fa_write(testdata.c_str(),testdata.length()));
  CPPUNIT_ASSERT(fa.fa_close());
  CPPUNIT_ASSERT(fa.fa_rename(oldfile, newfile));
  struct stat st;
  CPPUNIT_ASSERT(fa.fa_stat(newfile,st));
  CPPUNIT_ASSERT(!fa.fa_stat(oldfile,st));
}

void FileAccessTest::TestDir() {
  std::string testdir1 = testroot + "/dir1/dir2/dir3";
  std::string testdir2 = testroot + "/dir1/dir2/dir3/dir4";
  std::string testdir3 = testroot + "/dir1";
  Arc::FileAccess fa;
  CPPUNIT_ASSERT(fa.fa_setuid(uid,gid));
  CPPUNIT_ASSERT(!fa.fa_mkdir(testdir1,0700));
  CPPUNIT_ASSERT(fa.fa_mkdirp(testdir1,0700));
  CPPUNIT_ASSERT(fa.fa_mkdir(testdir2,0700));
  CPPUNIT_ASSERT(fa.fa_opendir(testdir1));
  std::string name;
  while(true) {
    CPPUNIT_ASSERT(fa.fa_readdir(name));
    if(name == ".") continue;
    if(name == "..") continue;
    break;
  }
  CPPUNIT_ASSERT(fa.fa_closedir());
  CPPUNIT_ASSERT_EQUAL(testdir2.substr(testdir1.length()+1),name);
  CPPUNIT_ASSERT(!fa.fa_rmdir(testdir3));
  CPPUNIT_ASSERT(fa.fa_rmdir(testdir2));
  CPPUNIT_ASSERT(fa.fa_rmdirr(testdir3));
}

void FileAccessTest::TestSeekAllocate() {
  Arc::FileAccess fa;
  std::string testfile = testroot+"/file3";
  CPPUNIT_ASSERT(fa.fa_setuid(uid,gid));
  CPPUNIT_ASSERT(fa.fa_open(testfile,O_WRONLY|O_CREAT|O_EXCL,0600));
  CPPUNIT_ASSERT_EQUAL((int)4096,(int)fa.fa_fallocate(4096));
  CPPUNIT_ASSERT_EQUAL((int)0,(int)fa.fa_lseek(0,SEEK_SET));
  CPPUNIT_ASSERT_EQUAL((int)4096,(int)fa.fa_lseek(0,SEEK_END));
  CPPUNIT_ASSERT(fa.fa_close());
}

CPPUNIT_TEST_SUITE_REGISTRATION(FileAccessTest);
