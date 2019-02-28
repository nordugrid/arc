#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "jura.h"

#include <iostream>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

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

static Arc::Logger logger(Arc::Logger::getRootLogger(),"JURA");

int main(int argc, char **argv)
{

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);

  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::LongFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

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
  while((n=getopt(argc,argv,":u:t:o:y:F:m:r:c:afsv")) != -1) {
    switch(n) {
    case ':': { logger.msg(Arc::ERROR, "Missing option argument"); return 1; }
    case '?': { logger.msg(Arc::ERROR, "Unrecognized option"); return 1; }
    case 'u':
      urls.push_back(std::string(optarg));
      topics.push_back("");
      break;
    case 't':
      if (topics.begin() == topics.end()){
          logger.msg(Arc::ERROR, "Add URL value before a topic. (for example: -u [...] -t [...])");
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
      logger.msg(Arc::INFO, "Force resend all aggregation records.");
      force_resend = true;
      break;
    case 's':
      logger.msg(Arc::INFO, "Sync message(s) will be send...");
      sync = true;
      break;
    case 'v':
      std::cout << Arc::IString("%s version %s", "jura", VERSION)
              << std::endl;
      return 0;
      break;
    case 'c':
      config_file = optarg;
      break;
    default: { logger.msg(Arc::ERROR, "Options processing error"); return 1; }
    }
  }
  
  // If configuration file specified parse it. And use configuration file instead
  // of command line aguments because this is how A-REX calls Jura. 
  ArcJura::Config config(config_file.empty() ? NULL : config_file.c_str());
  if (!config_file.empty() && !config) {
    logger.msg(Arc::ERROR, "Failed processing configuration file %s", config_file); return 1;
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
            logger.msg(Arc::ERROR, "ServiceURL missing");
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
            logger.msg(Arc::ERROR, "Topic missing for a (%s) host.", urls[i]);
            continue;
          }
        logger.msg(Arc::INFO, "Aggregation record(s) sending to %s", host);
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
      logger.msg(Arc::INFO, "resend opt: %s", resend_range);
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
