#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

int main() {
  CppUnit::TextUi::TestRunner runner;
  runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

  runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter
		      (&runner.result(), std::cerr));

  bool wasSuccessful = runner.run();

  return wasSuccessful ? 0 : 1;
}
