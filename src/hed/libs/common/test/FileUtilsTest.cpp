#include <sys/stat.h>
#include <cppunit/extensions/HelperMacros.h>

#include <arc/FileUtils.h>

class FileUtilsTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileUtilsTest);
  CPPUNIT_TEST(TestMakeAndDeleteDir);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

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

}

void FileUtilsTest::TestMakeAndDeleteDir() {
  // create a few subdirs and files then recursively delete
  struct stat st;
  CPPUNIT_ASSERT(stat(testroot.c_str(), &st) == 0);
  CPPUNIT_ASSERT(_createFile(testroot + "/file1"));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot + "/dir1").c_str(), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(stat(std::string(testroot + "/dir1").c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(_createFile(testroot + "/dir1/file2"));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot + "/dir1/dir2").c_str(), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(stat(std::string(testroot + "/dir1/dir2").c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(_createFile(testroot + "/dir1/dir2/file3"));

  CPPUNIT_ASSERT(Arc::DirDelete(testroot.c_str()));
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
