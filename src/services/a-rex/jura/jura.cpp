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

  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::rootLogger.addDestination(logcerr);

  opterr=0;
  std::vector<std::string> urls;
  std::vector<std::string> topics;
  std::string output_dir;
  bool aggregation  = false;
  bool sync = false;
  bool force_resend = false;
  bool ur_resend = false;
  std::string resend_range = "";
  std::string year  = "";
  std::string month = "";
  std::string vo_filters=""; 
  std::string config_file;
  int n;
  while((n=getopt(argc,argv,":u:t:o:y:F:m:r:c:afsvL")) != -1) {
    switch(n) {
    case ':': { std::cerr<<"Missing argument\n"; return 1; }
    case '?': { std::cerr<<"Unrecognized option\n"; return 1; }
    case 'u':
      urls.push_back(std::string(optarg));
      topics.push_back("");
      break;
    case 't':
      if (topics.begin() == topics.end()){
          std::cerr<<"Add URL value before a topic. (for example: -u [...] -t [...])\n";
          return -1;
      }
      topics.back() = optarg;
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
    case 'F':
      vo_filters = optarg;
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
    case 'L':
      logcerr.setFormat(Arc::LongFormat);
      break;
    case 'c':
      config_file = optarg;
      break;
    default: { std::cerr<<"Options processing error"<<std::endl; return 1; }
    }
  }
  
  // If configuration file specified parse it. And use configuration file instead
  // of command line aguments because this is how A-REX calls Jura. 
  ArcJura::Config config(config_file.empty() ? NULL : config_file.c_str());
  if (!config_file.empty() && !config) {
    std::cerr<<"Failed processing configuration file "<<config_file<<std::endl; return 1;
  }

  if ( aggregation ) {
    ArcJura::CARAggregation* aggr;
    for (int i=0; i<(int)urls.size(); i++)
      {
        std::cout << urls[i] << std::endl;
        //  Tokenize service URL
        std::string host, port, endpoint;
        if (urls[i].empty())
          {
            std::cerr << "ServiceURL missing" << std::endl;
            continue;
          }
        else
          {
            Arc::URL url(urls[i]);
            host=url.Host();
            std::ostringstream os;
            os<<url.Port();
            port=os.str();
            endpoint=url.Path();
          }

        if (topics[i].empty())
          {
            std::cerr << "Topic missing for a (" << urls[i] << ") host." << std::endl;
            continue;
          }
        std::cerr << "Aggregation record(s) sending to " << host << std::endl;
        aggr = new ArcJura::CARAggregation(host, port, topics[i], sync);

        if ( !year.empty() )
          {
            aggr->Reporting_records(year, month);
          }
        else 
          {
            aggr->Reporting_records(force_resend);
          }
        delete aggr;
      }
    return 0;
  }

  // The essence:
  ArcJura::Reporter *usagereporter;
  if ( ur_resend ) {
      std::cerr << "resend opt:" << resend_range << std::endl;
      usagereporter=new ArcJura::ReReporter(
                      config.getArchiveDir(),
                      resend_range, urls, topics, vo_filters );

      } else {
      usagereporter=new ArcJura::UsageReporter(config);
  }
  usagereporter->report();
  delete usagereporter;
  return 0;
}
