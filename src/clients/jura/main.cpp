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

  opterr=0;
  time_t ex_period = 0;
  std::list<std::string> urls;
  int n;
  while((n=getopt(argc,argv,":E:u:")) != -1) {
    switch(n) {
    case ':': { std::cerr<<"Missing argument\n"; return 1; }
    case '?': { std::cerr<<"Unrecognized option\n"; return 1; }
    case 'E': 
      {
        char* p;
        int i = strtol(optarg,&p,10);
        if(((*p) != 0) || (i<=0)) {
	  std::cerr<<"Improper expiration period '"<<optarg<<"'"<<std::endl;
          exit(1);
        }
        ex_period=i*(60*60*24);
      } 
      break;
    case 'u':
      urls.push_back(std::string(optarg));
      break;
    default: { std::cerr<<"Options processing error\n"; return 1; }
    }
  }

  // The essence:
  int argind;
  Arc::UsageReporter *usagereporter;
  for (argind=optind ; argind<argc ; ++argind)
    {
      usagereporter=new Arc::UsageReporter(
	                  std::string(argv[argind])+"/logs",
			  ex_period, urls );
      usagereporter->report();
      delete usagereporter;
    }
  return 0;
}
