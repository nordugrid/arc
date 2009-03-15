// -*- indent-tabs-mode: nil -*-

#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include <arc/Logger.h>

class LoggerTest
  : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(LoggerTest);
  CPPUNIT_TEST(TestLoggerINFO);
  CPPUNIT_TEST(TestLoggerDEBUG);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void TestLoggerINFO();
  void TestLoggerDEBUG();

private:
  std::stringstream stream;
  Arc::LogStream *output;
  Arc::Logger *logger;
};


void LoggerTest::setUp() {
  output = new Arc::LogStream(stream);
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
  logger->msg(Arc::DEBUG, "This DEBUG message should not be seen");
  res = stream.str();
  CPPUNIT_ASSERT(res.empty());

  logger->msg(Arc::INFO, "This INFO message should be seen");
  res = stream.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This INFO message should be seen\n"));
  stream.str("");
}


void LoggerTest::TestLoggerDEBUG() {
  std::string res;
  logger->setThreshold(Arc::DEBUG);
  logger->msg(Arc::DEBUG, "This DEBUG message should now be seen");
  res = stream.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This DEBUG message should now be seen\n"));
  stream.str("");

  logger->msg(Arc::INFO, "This INFO message should also be seen");
  res = stream.str();
  res = res.substr(res.rfind(']') + 2);
  CPPUNIT_ASSERT_EQUAL(res, std::string("This INFO message should also be seen\n"));
  stream.str("");
}


CPPUNIT_TEST_SUITE_REGISTRATION(LoggerTest);
