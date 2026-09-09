// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <sstream>
#include <thread>
#include <vector>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Logger.h>
#include <arc/FileUtils.h>

class LoggerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(LoggerTest);
  CPPUNIT_TEST(TestLoggerINFO);
  CPPUNIT_TEST(TestLoggerVERBOSE);
  CPPUNIT_TEST(TestLoggerTHREAD);
  CPPUNIT_TEST(TestLoggerDEFAULT);
  CPPUNIT_TEST(TestLogFile);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestLoggerINFO();
  void TestLoggerVERBOSE();
  void TestLoggerTHREAD();
  void TestLoggerDEFAULT();
  void TestLogFile();

private:
  std::stringstream stream;
  std::stringstream stream_thread;
  Arc::LogStream *output;
  Arc::LogStream *output_thread;
  Arc::Logger *logger;
  static void thread(void* arg);
  std::mutex thread_lock;
};


void LoggerTest::setUp() {
  output = new Arc::LogStream(stream);
  output_thread = new Arc::LogStream(stream_thread);
  Arc::Logger::getRootLogger().addDestination(*output);
  logger = new Arc::Logger(Arc::Logger::getRootLogger(), "TestLogger", Arc::INFO);
}

void LoggerTest::tearDown() {
  Arc::Logger::getRootLogger().removeDestinations();
  delete logger;
  delete output;
}

void LoggerTest::TestLoggerINFO() {
  std::string res;
  logger->msg(Arc::VERBOSE, "This VERBOSE message should not be seen");
  res = stream.str();
  CPPUNIT_ASSERT(res.empty());

  logger->msg(Arc::INFO, "This INFO message should be seen");
  res = stream.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This INFO message should be seen\n"));
  stream.str("");
}


void LoggerTest::TestLoggerVERBOSE() {
  std::string res;
  logger->setThreshold(Arc::VERBOSE);
  logger->msg(Arc::VERBOSE, "This VERBOSE message should now be seen");
  res = stream.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This VERBOSE message should now be seen\n"));
  stream.str("");

  logger->msg(Arc::INFO, "This INFO message should also be seen");
  res = stream.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This INFO message should also be seen\n"));
  stream.str("");
}

void LoggerTest::TestLoggerTHREAD() {
  std::string res;
  logger->setThreshold(Arc::VERBOSE);
  thread_lock.lock();
  Arc::CreateThreadFunction(&thread,this);
  thread_lock.lock();
  thread_lock.unlock();
  logger->msg(Arc::VERBOSE, "This message goes to initial destination");
  res = stream.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This message goes to initial destination\n"));
  stream.str("");
  res = stream_thread.str();
  CPPUNIT_ASSERT(res.empty());
}

void LoggerTest::thread(void* arg) {
  std::string res;
  LoggerTest& it = *((LoggerTest*)arg);
  Arc::Logger::getRootLogger().setThreadContext();
  Arc::Logger::getRootLogger().removeDestinations();
  Arc::Logger::getRootLogger().addDestination(*it.output_thread);
  it.logger->msg(Arc::VERBOSE, "This message goes to per-thread destination");
  res = it.stream_thread.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This message goes to per-thread destination\n"));
  it.stream_thread.str("");
  res = it.stream.str();
  CPPUNIT_ASSERT(res.empty());
  it.thread_lock.unlock();
}

void LoggerTest::TestLoggerDEFAULT() {
  Arc::LogLevel default_level = Arc::Logger::getRootLogger().getThreshold();
  Arc::LogLevel bad_level = Arc::istring_to_level("COW");
  CPPUNIT_ASSERT_EQUAL(bad_level, default_level);
}

void LoggerTest::TestLogFile() {
  std::string filename;
  CPPUNIT_ASSERT(Arc::TmpFileCreate(filename, ""));
  {
    Arc::LogFile file(filename);
    file.setFormat(Arc::EmptyFormat);
    file.log(Arc::LogMessage(Arc::INFO, Arc::IString("record")));
    std::string content;
    // Messages must be visible before destruction, including reopen mode.
    CPPUNIT_ASSERT(Arc::FileRead(filename, content));
    CPPUNIT_ASSERT_EQUAL(std::string("record\n"), content);
    file.setReopen(true);
    file.log(Arc::LogMessage(Arc::INFO, Arc::IString("record")));
    CPPUNIT_ASSERT(Arc::FileRead(filename, content));
    CPPUNIT_ASSERT_EQUAL(std::string("record\nrecord\n"), content);
    file.setReopen(false);

    std::vector<std::thread> writers;
    for (int i = 0; i < 4; ++i) {
      writers.push_back(std::thread([&file]() {
        for (int n = 0; n < 500; ++n)
          file.log(Arc::LogMessage(Arc::INFO, Arc::IString("record")));
      }));
    }
    for (std::vector<std::thread>::iterator i = writers.begin(); i != writers.end(); ++i)
      i->join();
    std::list<std::string> lines;
    CPPUNIT_ASSERT(Arc::FileRead(filename, lines));
    CPPUNIT_ASSERT_EQUAL(2002, (int)lines.size());
    for (std::list<std::string>::const_iterator i = lines.begin(); i != lines.end(); ++i)
      CPPUNIT_ASSERT_EQUAL(std::string("record"), *i);

    file.setMaxSize(1);
    file.setBackups(1);
    file.log(Arc::LogMessage(Arc::INFO, Arc::IString("rotated")));
    CPPUNIT_ASSERT(Arc::FileRead(filename+".1", content));
    CPPUNIT_ASSERT_EQUAL(std::string("rotated\n"), content.substr(content.size()-8));
  }
  CPPUNIT_ASSERT(Arc::FileDelete(filename));
  CPPUNIT_ASSERT(Arc::FileDelete(filename+".1"));
}

CPPUNIT_TEST_SUITE_REGISTRATION(LoggerTest);
