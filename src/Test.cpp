// -*- indent-tabs-mode: nil -*-

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "libarcdatatest");

/*
 * Use -v to enable logging when running a unit test directly.
 * E.g.: ./hed/libs/common/test/LoggerTest -v
 */
int main(int argc, char **argv) {

  Arc::LogStream logcerr(std::cerr);
  Arc::ArcLocation::Init(argv[0]);

  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    // set logging
    Arc::Logger::getRootLogger().addDestination(logcerr);
    Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  }

  CppUnit::TextUi::TestRunner runner;
  runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

  runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter
                      (&runner.result(), std::cerr));

  bool wasSuccessful = runner.run();

  return wasSuccessful ? 0 : 1;
}
