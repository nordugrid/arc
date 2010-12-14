// -*- indent-tabs-mode: nil -*-

#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Logger.h>

class LoggerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(LoggerTest);
  CPPUNIT_TEST(TestLoggerINFO);
  CPPUNIT_TEST(TestLoggerVERBOSE);
  CPPUNIT_TEST(TestLoggerTHREAD);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestLoggerINFO();
  void TestLoggerVERBOSE();
  void TestLoggerTHREAD();

private:
  std::stringstream stream;
  std::stringstream stream_thread;
  Arc::LogStream *output;
  Arc::LogStream *output_thread;
  Arc::Logger *logger;
  static void thread(void* arg);
  Glib::Mutex thread_lock;
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


CPPUNIT_TEST_SUITE_REGISTRATION(LoggerTest);
