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
  CPPUNIT_TEST(TestDirOpen);
  CPPUNIT_TEST(TestMakeAndDeleteDir);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestFileOpen();
  void TestFileStat();
  void TestFileCopy();
  void TestDirOpen();
  void TestMakeAndDeleteDir();

private:
  bool _createFile(const std::string& filename, const std::string& text = "a");
  std::string testroot;
};


void FileUtilsTest::setUp() {
  char tmpdirtemplate[] = "/tmp/ARC-Test-XXXXXX";
  char * tmpdir = mkdtemp(tmpdirtemplate);
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
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot + "/dir1/dir2"), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(stat(std::string(testroot + "/dir1/dir2").c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(_createFile(testroot + "/dir1/dir2/file3"));
  CPPUNIT_ASSERT(symlink(std::string(testroot + "/dir1/dir2").c_str(), std::string(testroot + "/dir1/dir2/link1").c_str()) == 0);

  CPPUNIT_ASSERT(Arc::DirDelete(testroot));
  CPPUNIT_ASSERT(stat(testroot.c_str(), &st) != 0);
  
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
