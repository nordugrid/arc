// -*- indent-tabs-mode: nil -*-

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <arc/Logger.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "libarcdatatest");

int main(int argc, char **argv) {

  Arc::LogStream logcerr(std::cerr);

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
