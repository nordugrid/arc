#include <fcntl.h>
#include <sys/stat.h>
#include <cppunit/extensions/HelperMacros.h>

#include <arc/FileUtils.h>

class FileUtilsTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileUtilsTest);
  CPPUNIT_TEST(TestFileOpen);
  CPPUNIT_TEST(TestFileStat);
  CPPUNIT_TEST(TestFileCopy);
  CPPUNIT_TEST(TestFileLink);
  CPPUNIT_TEST(TestFileCreateAndRead);
  CPPUNIT_TEST(TestDirOpen);
  CPPUNIT_TEST(TestMakeAndDeleteDir);
  CPPUNIT_TEST(TestTmpDirCreate);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestFileOpen();
  void TestFileStat();
  void TestFileCopy();
  void TestFileLink();
  void TestFileCreateAndRead();
  void TestDirOpen();
  void TestMakeAndDeleteDir();
  void TestTmpDirCreate();

private:
  bool _createFile(const std::string& filename, const std::string& text = "a");
  std::string testroot;
};


void FileUtilsTest::setUp() {
  std::string tmpdir;
  Arc::TmpDirCreate(tmpdir);
  testroot = tmpdir;
}

void FileUtilsTest::tearDown() {
  Arc::DirDelete(testroot);
}

void FileUtilsTest::TestFileOpen() {
  int h = Arc::FileOpen(testroot+"/file1", O_WRONLY | O_CREAT);
  CPPUNIT_ASSERT(h > 0);
  CPPUNIT_ASSERT_EQUAL(0, close(h));
  h = Arc::FileOpen(testroot+"/file1", O_WRONLY | O_CREAT, -1, -1, S_IRUSR | S_IWUSR);
  CPPUNIT_ASSERT(h < 0);
}

void FileUtilsTest::TestFileStat() {
  int h = Arc::FileOpen(testroot+"/file1", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  CPPUNIT_ASSERT(h > 0);
  CPPUNIT_ASSERT_EQUAL(0, close(h));
  struct stat st;
  CPPUNIT_ASSERT(Arc::FileStat(testroot+"/file1", &st, true));
  CPPUNIT_ASSERT_EQUAL(0, (int)st.st_size);
  CPPUNIT_ASSERT(S_IRUSR & st.st_mode);
  CPPUNIT_ASSERT(S_IWUSR & st.st_mode);
  CPPUNIT_ASSERT(!Arc::FileStat(testroot+"/file2", &st, true));
  CPPUNIT_ASSERT(!Arc::FileStat(testroot+"/file1", &st, -1, -1, true));
}

void FileUtilsTest::TestFileCopy() {
  CPPUNIT_ASSERT(_createFile(testroot + "/file1"));
  CPPUNIT_ASSERT(Arc::FileCopy(testroot+"/file1", testroot+"/file2"));
  struct stat st;
  CPPUNIT_ASSERT(Arc::FileStat(testroot+"/file2", &st, true));
  CPPUNIT_ASSERT_EQUAL(1, (int)st.st_size);
  int h = Arc::FileOpen(testroot+"/file2", O_RDONLY , S_IRUSR | S_IWUSR);
  CPPUNIT_ASSERT(h > 0);
  int h2 = Arc::FileOpen(testroot+"/file3", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  CPPUNIT_ASSERT(h > 0);
  CPPUNIT_ASSERT(Arc::FileCopy(h, h2));
  CPPUNIT_ASSERT_EQUAL(0, close(h));
  CPPUNIT_ASSERT_EQUAL(0, close(h2));
}

void FileUtilsTest::TestFileLink() {
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

void FileUtilsTest::TestFileCreateAndRead() {
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
  CPPUNIT_ASSERT_EQUAL(0, remove(filename.c_str()));
  CPPUNIT_ASSERT(!Arc::FileRead(filename, data));
}

void FileUtilsTest::TestDirOpen() {
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

void FileUtilsTest::TestMakeAndDeleteDir() {
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

void FileUtilsTest::TestTmpDirCreate() {
  std::string path;
  CPPUNIT_ASSERT(Arc::TmpDirCreate(path));
  struct stat st;
  CPPUNIT_ASSERT(stat(path.c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(Arc::DirDelete(path));
  CPPUNIT_ASSERT(stat(path.c_str(), &st) != 0);
}

bool FileUtilsTest::_createFile(const std::string& filename, const std::string& text) {

  FILE *pFile;
  pFile = fopen((char*)filename.c_str(), "w");
  if (pFile == NULL) return false;
  fputs((char*)text.c_str(), pFile);
  fclose(pFile);
  return true;
}

CPPUNIT_TEST_SUITE_REGISTRATION(FileUtilsTest);
