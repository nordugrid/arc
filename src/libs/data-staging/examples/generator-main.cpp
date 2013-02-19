#include <signal.h>
#include <arc/StringConv.h>
#include "Generator.h"

static Arc::SimpleCounter counter;
static bool run = true;

static void do_shutdown(int) {
  run = false;
}

static void usage() {
  std::cout << "Usage: generator [num mock transfers]" << std::endl;
  std::cout << "       generator source destination" << std::endl;
  std::cout << "To use mock transfers ARC must be built with configure --enable-mock-dmc" << std::endl;
  std::cout << "The default number of mock transfers is 10" << std::endl;
}

int main(int argc, char** argv) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGINT, do_shutdown);

  // Log to stderr
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::INFO);

  Generator generator;
  int num = 10;
  if (argc == 1 || argc == 2) { // run mock a number of times
    if (argc == 2 && (std::string(argv[1]) == "-h" || !Arc::stringto(argv[1], num))) {
      usage();
      return 1;
    }
    generator.start();
    for (int i = 0; i < num; ++i) {
      std::string source = "mock://mocksrc/mock." + Arc::tostring(i);
      std::string destination = "mock://mockdest/mock." + Arc::tostring(i);
      generator.run(source, destination);
    }
  }
  else if (argc == 3) { // run with given source and destination
    generator.start();
    generator.run(argv[1], argv[2]);
  }
  else {
    usage();
    return 1;
  }

  while (generator.counter.get() > 0 && run) {
    sleep(1);
  }
  return 0;
}
