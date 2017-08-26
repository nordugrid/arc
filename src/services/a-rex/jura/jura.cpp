#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "jura.h"

#include <iostream>
//TODO cross-platform
#include <signal.h>
#include <errno.h>
#include <unistd.h>


//TODO cross-platform
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sstream>

#ifdef WIN32
#include <arc/win32.h>
#endif
#include <arc/Logger.h>
#include <arc/Utils.h>

#include "Reporter.h"
#include "UsageReporter.h"
#include "ReReporter.h"
#include "CARAggregation.h"
#include "Config.h"


int main(int argc, char **argv)
{

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);

  opterr=0;
  time_t ex_period = 0;
  std::string output_dir;
  bool aggregation  = false;
  bool sync = false;
  bool force_resend = false;
  bool ur_resend = false;
  std::string resend_range = "";
  std::string year  = "";
  std::string month = "";
  std::string config_file;
  int n;
  while((n=getopt(argc,argv,":E:o:y:m:r:c:afsv")) != -1) {
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
    case 'o':
      output_dir = optarg;
      break;
    case 'a':
      aggregation = true;
      break;
    case 'y':
      year = optarg;
      break;
    case 'm':
      month = optarg;
      break;
    case 'r':
      ur_resend = true;
      resend_range = optarg;
      break;
    case 'f':
      std::cout << "Force resend all aggregation records." << std::endl;
      force_resend = true;
      break;
    case 's':
      std::cout << "Sync message(s) will be send..." << std::endl;
      sync = true;
      break;
    case 'v':
      std::cout << Arc::IString("%s version %s", "jura", VERSION)
              << std::endl;
      return 0;
      break;
    case 'c':
      config_file = optarg;
    default: { std::cerr<<"Options processing error"<<std::endl; return 1; }
    }
  }
  
  if(config_file.empty()) config_file = Arc::GetEnv("ARC_CONFIG");
  if(config_file.empty()) config_file = "/etc/arc.conf";
  ArcJura::Config config(config_file.c_str());
  if(!config) {
    std::cerr<<"Configuration file processing error"<<std::endl; return 1;
  }
  if(config.getLogfile().empty()) {
    std::cerr<<"Logging destination is not set"<<std::endl; return 1;
  }
  Arc::LogFile logfile(config.getLogfile());
  logfile.setFormat(Arc::LongFormat);
  Arc::Logger::rootLogger.setThreshold(config.getLoglevel());
  Arc::Logger::rootLogger.addDestination(logfile);


  if ( aggregation ) {
    ArcJura::CARAggregation* aggr;
    for (int i=0; i<(int)config.getAPEL().size(); i++)
      {
        ArcJura::Config::APEL const & apel(config.getAPEL()[i]);
        std::cout << apel.targeturl.str() << std::endl;
        //  Tokenize service URL
        std::string host, port, endpoint;
        if (!apel.targeturl)
          {
            std::cerr << "ServiceURL missing" << std::endl;
            continue;
          }
        else
          {
            Arc::URL const & url(apel.targeturl);
            host=url.Host();
            std::ostringstream os;
            os<<url.Port();
            port=os.str();
            endpoint=url.Path();
          }

        if (apel.topic.empty())
          {
            std::cerr << "Topic missing for a (" << apel.targeturl << ") host." << std::endl;
            continue;
          }
        std::cerr << "Aggregation record(s) sending to " << host << std::endl;
        ArcJura::CARAggregation aggr(host, port, apel.topic, sync);

        if ( !year.empty() )
          {
            aggr.Reporting_records(year, month);
          }
        else 
          {
            aggr.Reporting_records(force_resend);
          }
      }
    return 0;
  }

  // The essence:
  int argind;
  ArcJura::Reporter *usagereporter;
  for (argind=optind ; argind<argc ; ++argind)
    {
      if ( ur_resend ) {
          std::cerr << "resend opt:" << resend_range << std::endl;
          usagereporter=new ArcJura::ReReporter(config,
                          std::string(argv[argind]),
                          resend_range );

      } else {
          usagereporter=new ArcJura::UsageReporter(config,
                          std::string(argv[argind])+"/logs",
                          ex_period, output_dir );
      }
      usagereporter->report();
      delete usagereporter;
    }
  return 0;
}
