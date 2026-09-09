// TODO: test for operations under different account

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cerrno>
#include <csignal>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <cppunit/extensions/HelperMacros.h>

#include <glibmm/miscutils.h>

#include <arc/FileAccess.h>
#include <arc/FileUtils.h>

class FileUtilsTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileUtilsTest);
  CPPUNIT_TEST(TestFileStat);
  CPPUNIT_TEST(TestFileCopy);
  CPPUNIT_TEST(TestLargeFileCopy);
  CPPUNIT_TEST(TestFileLink);
  CPPUNIT_TEST(TestFileCreateAndRead);
  CPPUNIT_TEST(TestFileCreateFailure);
  CPPUNIT_TEST(TestMakeAndDeleteDir);
  CPPUNIT_TEST(TestRecursiveCleanup);
  CPPUNIT_TEST(TestSelectiveCleanup);
  CPPUNIT_TEST(TestTmpDirCreate);
  CPPUNIT_TEST(TestTmpFileCreate);
  CPPUNIT_TEST(TestDirList);
  CPPUNIT_TEST(TestCanonicalDir);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestFileStat();
  void TestFileCopy();
  void TestLargeFileCopy();
  void TestFileLink();
  void TestFileCreateAndRead();
  void TestFileCreateFailure();
  void TestMakeAndDeleteDir();
  void TestRecursiveCleanup();
  void TestSelectiveCleanup();
  void TestTmpDirCreate();
  void TestTmpFileCreate();
  void TestDirList();
  void TestCanonicalDir();

private:
  bool _createFile(const std::string& filename, const std::string& text = "a");
  std::string testroot;
  std::string sep;
};


void FileUtilsTest::setUp() {
  std::string tmpdir;
  Arc::TmpDirCreate(tmpdir);
  testroot = tmpdir;
  sep = G_DIR_SEPARATOR_S;
  Arc::FileAccess::testtune();
}

void FileUtilsTest::tearDown() {
  Arc::DirDelete(testroot);
}

void FileUtilsTest::TestFileStat() {
  CPPUNIT_ASSERT(_createFile(testroot + "/file1"));
  struct stat st;
  CPPUNIT_ASSERT(Arc::FileStat(testroot+"/file1", &st, true));
  CPPUNIT_ASSERT_EQUAL(1, (int)st.st_size);
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
  int h = open(std::string(testroot+"/file2").c_str(), O_RDONLY , S_IRUSR | S_IWUSR);
  CPPUNIT_ASSERT(h > 0);
  int h2 = open(std::string(testroot+"/file3").c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  CPPUNIT_ASSERT(h > 0);
  CPPUNIT_ASSERT(Arc::FileCopy(h, h2));
  CPPUNIT_ASSERT_EQUAL(0, close(h));
  CPPUNIT_ASSERT_EQUAL(0, close(h2));
}

void FileUtilsTest::TestLargeFileCopy() {
  // Exceed the mmap threshold and end with a short buffer, including NULs.
  std::string content(51*1024*1024 + 17, '\0');
  for (std::size_t i = 0; i < content.size(); ++i)
    content[i] = static_cast<char>(i % 251);
  const std::string source = testroot + "/large-source";
  const std::string target = testroot + "/large-target";
  CPPUNIT_ASSERT(Arc::FileCreate(source, content));
  CPPUNIT_ASSERT(Arc::FileCopy(source, target));
  std::string result;
  CPPUNIT_ASSERT(Arc::FileRead(target, result));
  CPPUNIT_ASSERT(content == result);

  // A write failure after reading the first buffer must still be reported,
  // without closing descriptors owned by the caller or modifying the target.
  int in = open(source.c_str(), O_RDONLY);
  int out = open(target.c_str(), O_RDONLY);
  CPPUNIT_ASSERT(in >= 0 && out >= 0);
  const bool copied = Arc::FileCopy(in, out);
  const int close_in = close(in), close_out = close(out);
  CPPUNIT_ASSERT(!copied);
  CPPUNIT_ASSERT_EQUAL(0, close_in);
  CPPUNIT_ASSERT_EQUAL(0, close_out);
  CPPUNIT_ASSERT(Arc::FileRead(target, result));
  CPPUNIT_ASSERT(content == result);
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
  CPPUNIT_ASSERT(Arc::FileCreate(filename, "", 0, 0, 0600));

  struct stat st;
  CPPUNIT_ASSERT(Arc::FileStat(filename, &st, true));
  CPPUNIT_ASSERT_EQUAL(0, (int)st.st_size);
  CPPUNIT_ASSERT_EQUAL(0600, (int)(st.st_mode & 0777));

  std::list<std::string> data;
  CPPUNIT_ASSERT(Arc::FileRead(filename, data));
  CPPUNIT_ASSERT(data.empty());

  // create again with some data
  CPPUNIT_ASSERT(Arc::FileCreate(filename, "12\nabc\n\nxyz\n"));

  CPPUNIT_ASSERT(Arc::FileRead(filename, data));
  CPPUNIT_ASSERT_EQUAL(4, (int)data.size());
  CPPUNIT_ASSERT_EQUAL(std::string("12"), data.front());
  CPPUNIT_ASSERT_EQUAL(std::string("xyz"), data.back());
  std::string cdata;
  CPPUNIT_ASSERT(Arc::FileRead(filename, cdata));
  CPPUNIT_ASSERT_EQUAL(std::string("12\nabc\n\nxyz\n"), cdata);

  // Multiple read buffers, a short last chunk and embedded NUL bytes.
  std::string binary(1024*1024 + 17, '\0');
  for (std::string::size_type i = 0; i < binary.size(); ++i)
    binary[i] = static_cast<char>(i % 256);
  CPPUNIT_ASSERT(Arc::FileCreate(filename, binary));
  CPPUNIT_ASSERT(Arc::FileRead(filename, cdata));
  CPPUNIT_ASSERT_EQUAL(binary, cdata);

  // remove file and check failure
  CPPUNIT_ASSERT_EQUAL(true, Arc::FileDelete(filename.c_str()));
  CPPUNIT_ASSERT(!Arc::FileRead(filename, data));
}

void FileUtilsTest::TestFileCreateFailure() {
  const std::string target = testroot + "/control";
  CPPUNIT_ASSERT(Arc::FileCreate(target, "previous contents"));
  {
    // Force a partial write followed by EFBIG, restoring process state even
    // if an assertion fails. The old control file must not be replaced.
    struct LimitGuard {
      struct rlimit saved;
      typedef void (*Handler)(int);
      Handler handler;
      LimitGuard() {
        CPPUNIT_ASSERT_EQUAL(0, getrlimit(RLIMIT_FSIZE, &saved));
        handler = std::signal(SIGXFSZ, SIG_IGN);
      }
      ~LimitGuard() { setrlimit(RLIMIT_FSIZE, &saved); std::signal(SIGXFSZ, handler); }
    } guard;
    struct rlimit limit = guard.saved;
    limit.rlim_cur = 128;
    CPPUNIT_ASSERT_EQUAL(0, setrlimit(RLIMIT_FSIZE, &limit));
    CPPUNIT_ASSERT(!Arc::FileCreate(target, std::string(1024, 'x')));
    CPPUNIT_ASSERT_EQUAL(EFBIG, errno);
  }
  std::string content;
  CPPUNIT_ASSERT(Arc::FileRead(target, content));
  CPPUNIT_ASSERT_EQUAL(std::string("previous contents"), content);
  std::list<std::string> files;
  CPPUNIT_ASSERT(Arc::DirList(testroot, files, false));
  CPPUNIT_ASSERT_EQUAL(std::size_t(1), files.size());
  CPPUNIT_ASSERT_EQUAL(target, files.front());
}

void FileUtilsTest::TestMakeAndDeleteDir() {
  // create a few subdirs and files then recursively delete
  struct stat st;
  std::string file1("file1");
  std::string file2("file2");
  std::string file3("dir1"+sep+"file3");
  std::string file4("dir1"+sep+"dir3"+sep+"dir4"+sep+"file4");
  std::string dir1("dir1");
  std::string dir2("dir1"+sep+"dir2");
  std::string dir3("dir1"+sep+"dir3"+sep+"dir4");
  std::string link1("dir1"+sep+"dir2"+sep+"link1");

  CPPUNIT_ASSERT(stat(testroot.c_str(), &st) == 0);
  CPPUNIT_ASSERT(_createFile(testroot+sep+file1));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot+sep+dir1), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot+sep+dir1), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+dir1).c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(_createFile(testroot+sep+file2));
  CPPUNIT_ASSERT(_createFile(testroot+sep+file3));
  // should fail if with_parents is set to false
  CPPUNIT_ASSERT(!Arc::DirCreate(std::string(testroot+sep+dir3), S_IRUSR | S_IWUSR | S_IXUSR, false));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot+sep+dir3), S_IRUSR | S_IWUSR | S_IXUSR, true));
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+dir3).c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISDIR(st.st_mode));
  CPPUNIT_ASSERT(_createFile(testroot+sep+file4));
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot+sep+dir2), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(symlink(std::string(testroot+sep+file3).c_str(), std::string(testroot+sep+link1).c_str()) == 0);

  std::list<std::string> files;
  files.push_back("/");
  CPPUNIT_ASSERT(Arc::DirDeleteExcl(testroot, files, true));
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+dir2).c_str(), &st) == 0);
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+link1).c_str(), &st) == 0);

  files.clear();
  files.push_back("/" + file1);
  files.push_back("/" + dir2);
  // Delete everything except file1 and dir2
  CPPUNIT_ASSERT(Arc::DirDeleteExcl(testroot, files, true));
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+dir3).c_str(), &st) != 0);
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+dir2).c_str(), &st) == 0);
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+link1).c_str(), &st) != 0);
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+file1).c_str(), &st) == 0);
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+file2).c_str(), &st) != 0);

  files.pop_back();
  // Delete only file 1
  CPPUNIT_ASSERT(Arc::DirDeleteExcl(testroot, files, false));
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+dir2).c_str(), &st) == 0);
  CPPUNIT_ASSERT(stat(std::string(testroot+sep+file1).c_str(), &st) != 0);

  CPPUNIT_ASSERT(!Arc::DirDelete(testroot, false));
  CPPUNIT_ASSERT(Arc::DirDelete(testroot, true));
  CPPUNIT_ASSERT(stat(testroot.c_str(), &st) != 0);

}

void FileUtilsTest::TestRecursiveCleanup() {
  const std::string tree = testroot + "/tree";
  const std::string retained = testroot + "/retained";
  CPPUNIT_ASSERT(Arc::DirCreate(tree + "/nested/deep", 0700, true));
  CPPUNIT_ASSERT(Arc::DirCreate(tree + "/empty", 0700));
  CPPUNIT_ASSERT(Arc::DirCreate(retained, 0700));
  CPPUNIT_ASSERT(_createFile(retained + "/keep"));
  CPPUNIT_ASSERT(_createFile(tree + "/nested/deep/file"));
  CPPUNIT_ASSERT_EQUAL(0, chmod((tree + "/nested/deep/file").c_str(), 0400));
  CPPUNIT_ASSERT_EQUAL(0, symlink(retained.c_str(), (tree + "/link").c_str()));
  CPPUNIT_ASSERT_EQUAL(0, symlink("missing", (tree + "/dangling").c_str()));
  CPPUNIT_ASSERT_EQUAL(0, mkfifo((tree + "/fifo").c_str(), 0600));
  CPPUNIT_ASSERT(Arc::DirDelete(tree));
  struct stat st;
  CPPUNIT_ASSERT(!Arc::FileStat(tree, &st, false));
  CPPUNIT_ASSERT(Arc::FileStat(retained + "/keep", &st, false));
  CPPUNIT_ASSERT(!Arc::DirDelete(tree)); // missing root still reports failure
  CPPUNIT_ASSERT(!Arc::DirDelete(retained + "/keep")); // regular root is not a directory
  if (getuid() != 0) {
    CPPUNIT_ASSERT(Arc::DirCreate(tree, 0700));
    CPPUNIT_ASSERT(_createFile(tree + "/keep"));
    CPPUNIT_ASSERT_EQUAL(0, chmod(tree.c_str(), 0500));
    const bool deleted = Arc::DirDelete(tree);
    CPPUNIT_ASSERT_EQUAL(0, chmod(tree.c_str(), 0700));
    CPPUNIT_ASSERT(!deleted);
    CPPUNIT_ASSERT(Arc::FileStat(tree + "/keep", &st, false));
  }
}

void FileUtilsTest::TestSelectiveCleanup() {
  CPPUNIT_ASSERT(Arc::DirCreate(testroot + "/selected/nested", 0700, true));
  CPPUNIT_ASSERT(Arc::DirCreate(testroot + "/selected-other", 0700));
  CPPUNIT_ASSERT(_createFile(testroot + "/selected/nested/delete"));
  CPPUNIT_ASSERT(_createFile(testroot + "/selected/nested/keep"));
  CPPUNIT_ASSERT(_createFile(testroot + "/selected-other/keep"));
  CPPUNIT_ASSERT(_createFile(testroot + "/keep"));
  CPPUNIT_ASSERT_EQUAL(0, symlink("selected", (testroot + "/link").c_str()));
  CPPUNIT_ASSERT_EQUAL(0, symlink("missing", (testroot + "/dangling").c_str()));
  std::list<std::string> files;
  files.push_back("/selected/nested/delete");
  files.push_back("/selected/nested/delete"); // duplicates do not invert selection
  files.push_back("/missing");
  CPPUNIT_ASSERT(Arc::DirDeleteExcl(testroot, files, false));
  struct stat st;
  CPPUNIT_ASSERT(!Arc::FileStat(testroot + "/selected/nested/delete", &st, false));
  CPPUNIT_ASSERT(Arc::FileStat(testroot + "/selected/nested/keep", &st, false));
  CPPUNIT_ASSERT(Arc::FileStat(testroot + "/selected-other/keep", &st, false));
  CPPUNIT_ASSERT(Arc::FileStat(testroot + "/keep", &st, false));
  CPPUNIT_ASSERT(Arc::FileStat(testroot + "/link", &st, false) && S_ISLNK(st.st_mode));
  CPPUNIT_ASSERT(Arc::FileStat(testroot + "/dangling", &st, false) && S_ISLNK(st.st_mode));
  // Selecting a symlink must remove the link, not traverse its target.
  files.clear();
  files.push_back("/link");
  CPPUNIT_ASSERT(Arc::DirDeleteExcl(testroot, files, false));
  CPPUNIT_ASSERT(!Arc::FileStat(testroot + "/link", &st, false));
  CPPUNIT_ASSERT(Arc::FileStat(testroot + "/selected/nested/keep", &st, false));
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

void FileUtilsTest::TestTmpFileCreate() {
  // No specified path - uses system tmp
  std::string path;
  CPPUNIT_ASSERT(Arc::TmpFileCreate(path,"TEST"));
  struct stat st;
  CPPUNIT_ASSERT(stat(path.c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISREG(st.st_mode));
  CPPUNIT_ASSERT_EQUAL(4,(int)st.st_size);
  CPPUNIT_ASSERT(Arc::FileDelete(path));
  CPPUNIT_ASSERT(stat(path.c_str(), &st) != 0);

  // Specified path
  path = Glib::build_filename(Glib::get_tmp_dir(), "myfile-XXXXXX");
  CPPUNIT_ASSERT(Arc::TmpFileCreate(path,"TEST"));
  CPPUNIT_ASSERT_EQUAL(0, (int)path.find(Glib::build_filename(Glib::get_tmp_dir(), "myfile-")));
  CPPUNIT_ASSERT(stat(path.c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISREG(st.st_mode));
  CPPUNIT_ASSERT_EQUAL(4,(int)st.st_size);
  CPPUNIT_ASSERT(Arc::FileDelete(path));
  CPPUNIT_ASSERT(stat(path.c_str(), &st) != 0);

  // Specified path with no template - should use default name
  path = Glib::build_filename(Glib::get_tmp_dir(), "myfile");
  CPPUNIT_ASSERT(Arc::TmpFileCreate(path,"TEST"));
  CPPUNIT_ASSERT_EQUAL((int)std::string::npos, (int)path.find(Glib::build_filename(Glib::get_tmp_dir(), "myfile")));
  CPPUNIT_ASSERT(stat(path.c_str(), &st) == 0);
  CPPUNIT_ASSERT(S_ISREG(st.st_mode));
  CPPUNIT_ASSERT_EQUAL(4,(int)st.st_size);
  CPPUNIT_ASSERT(Arc::FileDelete(path));
  CPPUNIT_ASSERT(stat(path.c_str(), &st) != 0);
}

void FileUtilsTest::TestDirList() {
  // create a few subdirs and files then list
  struct stat st;
  std::list<std::string> entries;
  CPPUNIT_ASSERT(stat(testroot.c_str(), &st) == 0);
  CPPUNIT_ASSERT(Arc::DirCreate(std::string(testroot+sep+"dir1"), S_IRUSR | S_IWUSR | S_IXUSR));
  CPPUNIT_ASSERT(_createFile(testroot+sep+"dir1"+sep+"file1"));
  CPPUNIT_ASSERT(_createFile(testroot+sep+"file1"));

  // No such dir
  CPPUNIT_ASSERT(!Arc::DirList(std::string(testroot+sep+"test"), entries, false));
  CPPUNIT_ASSERT(entries.empty());

  // Not a dir
  CPPUNIT_ASSERT(!Arc::DirList(std::string(testroot+sep+"file1"), entries, false));
  CPPUNIT_ASSERT(entries.empty());

  // Should only list top-level
  CPPUNIT_ASSERT(Arc::DirList(std::string(testroot), entries, false));
  CPPUNIT_ASSERT_EQUAL(2, (int)entries.size());
  // The order of entries is not guaranteed
  CPPUNIT_ASSERT(std::find(entries.begin(), entries.end(), std::string(testroot+sep+"dir1")) != entries.end());
  CPPUNIT_ASSERT(std::find(entries.begin(), entries.end(), std::string(testroot+sep+"file1")) != entries.end());

  // List recursively
  CPPUNIT_ASSERT(Arc::DirList(std::string(testroot), entries, true));
  CPPUNIT_ASSERT_EQUAL(3, (int)entries.size());
  // The order of entries is not guaranteed
  CPPUNIT_ASSERT(std::find(entries.begin(), entries.end(), std::string(testroot+sep+"dir1")) != entries.end());
  CPPUNIT_ASSERT(std::find(entries.begin(), entries.end(), std::string(testroot+sep+"file1")) != entries.end());
  CPPUNIT_ASSERT(std::find(entries.begin(), entries.end(), std::string(testroot+sep+"dir1"+sep+"file1")) != entries.end());

  // Listing names must include dangling links and never recurse into symlinks.
  CPPUNIT_ASSERT_EQUAL(0, symlink("missing", (testroot+sep+"dangling").c_str()));
  CPPUNIT_ASSERT_EQUAL(0, symlink("dir1", (testroot+sep+"dirlink").c_str()));
  CPPUNIT_ASSERT(Arc::DirList(testroot, entries, false));
  CPPUNIT_ASSERT_EQUAL(4, (int)entries.size());
  CPPUNIT_ASSERT(Arc::DirList(testroot, entries, true));
  CPPUNIT_ASSERT_EQUAL(5, (int)entries.size());
}

void FileUtilsTest::TestCanonicalDir() {
  std::string dir(sep+"home"+sep+"me"+sep+"dir1");

  CPPUNIT_ASSERT(Arc::CanonicalDir(dir));
  CPPUNIT_ASSERT_EQUAL(std::string(sep+"home"+sep+"me"+sep+"dir1"), dir);

  CPPUNIT_ASSERT(Arc::CanonicalDir(dir, false));
  CPPUNIT_ASSERT_EQUAL(std::string("home"+sep+"me"+sep+"dir1"), dir);

  dir = sep+"home"+sep+"me"+sep+".."+sep+"me";
  CPPUNIT_ASSERT(Arc::CanonicalDir(dir));
  CPPUNIT_ASSERT_EQUAL(std::string(sep+"home"+sep+"me"), dir);

  dir = sep+"home"+sep+"me"+sep+".."+sep+"..";
  CPPUNIT_ASSERT(Arc::CanonicalDir(dir));
  CPPUNIT_ASSERT_EQUAL(sep, dir);

  dir = sep+"home"+sep+"me"+sep+".."+sep+"..";
  CPPUNIT_ASSERT(Arc::CanonicalDir(dir, false));
  CPPUNIT_ASSERT_EQUAL(std::string(""), dir);

  dir = sep+"home"+sep+"me"+sep+".."+sep+".."+sep+"..";
  CPPUNIT_ASSERT(!Arc::CanonicalDir(dir, false));

  dir = sep+"home"+sep+"me"+sep;
  CPPUNIT_ASSERT(Arc::CanonicalDir(dir, true, true));
  CPPUNIT_ASSERT_EQUAL(std::string(sep+"home"+sep+"me"+sep), dir);
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
