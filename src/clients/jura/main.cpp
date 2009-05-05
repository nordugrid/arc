#include "jura.h"

#include <iostream>
//TODO cross-platform
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "UsageReporter.h"


int main(int argc, char **argv)
{

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  //TODO parse command line!


  std::cerr<<"start\n";
  // The essence:
  Arc::UsageReporter usagereporter("/tmp/jobstatus/logs",1);
  usagereporter.report();
  // End of the essence
  std::cerr<<"stop\n";
  
  return 0;
}
