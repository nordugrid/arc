#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/extensions/HelperMacros.h>

#include <fcntl.h>
#include <unistd.h>
#include <arc/message/PayloadStream.h>

class MessageTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(MessageTest);
  CPPUNIT_TEST(TestPayloadStream);
  CPPUNIT_TEST_SUITE_END();

public:
  void TestPayloadStream();
};

void MessageTest::TestPayloadStream() {
  char buf[64];
  char const* testData = "0123456789abcdef";
  char const* testFileName = "TestFile.tmp";
  /*
  {
  // Read till EOF from ordinary file
  std::ofstream(testFileName, std::ios::trunc) << testData;
  int h = ::open(testFileName, O_RDONLY);
  Arc::PayloadStream inFile(h);
  int size = sizeof(buf);
  CPPUNIT_ASSERT(inFile.Get(buf, size));
  CPPUNIT_ASSERT(size == 16);
  CPPUNIT_ASSERT(memcmp(buf, testData, size) == 0);
  size = 1;
  CPPUNIT_ASSERT(!inFile.Get(buf, size));
  CPPUNIT_ASSERT(size == 0);
  ::close(h);
  ::unlink(testFileName);
  }

  {
  // Read till EOF from non-seekable pipe (assuming pipe buffer size is at least 16 bytes)
  int pipeFds[2];
  CPPUNIT_ASSERT(pipe2(pipeFds, 0) == 0);
  CPPUNIT_ASSERT(::write(pipeFds[1], testData, 16) == 16);
  ::close(pipeFds[1]);
  Arc::PayloadStream inStream(pipeFds[0]);
  int size = sizeof(buf);
  CPPUNIT_ASSERT(inStream.Get(buf, size));
  CPPUNIT_ASSERT(size == 16);
  CPPUNIT_ASSERT(memcmp(buf, testData, size) == 0);
  size = 1;
  CPPUNIT_ASSERT(!inStream.Get(buf, size));
  CPPUNIT_ASSERT(size == 0);
  ::close(pipeFds[0]);
  }
  */
  {
  // Read from file deleted in process
  std::ofstream(testFileName, std::ios::trunc) << testData;
  int h = ::open(testFileName, O_RDONLY);
  Arc::PayloadStream inFile(h);
  int size = 8;
  CPPUNIT_ASSERT(inFile.Get(buf, size));
  CPPUNIT_ASSERT(size == 8);
  CPPUNIT_ASSERT(memcmp(buf, testData, size) == 0);
  //::unlink(testFileName);
  truncate(testFileName, 8);
  size = 8;
  CPPUNIT_ASSERT(!inFile.Get(buf, size));
  CPPUNIT_ASSERT(size == 0);
  ::close(h);
  }

}

CPPUNIT_TEST_SUITE_REGISTRATION(MessageTest);
