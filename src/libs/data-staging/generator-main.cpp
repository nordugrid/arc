#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>

#include "Generator.h"

int main(int argc, char** argv) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);

  // Log to stderr
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  std::string source("http://localhost/file1");
  std::string destination("/tmp/file1");

  if (argc == 3) {
    source = argv[1];
    destination = argv[2];
  }

  DataStaging::Generator generator;
  generator.run(source, destination);

  return 0;
}
