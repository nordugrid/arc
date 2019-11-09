#ifndef _USAGEREPORTER_H
#define _USAGEREPORTER_H

#include <time.h>

#include <vector>
#include <string>

#include <arc/Logger.h>

#include "Reporter.h"
#include "Destinations.h"
#include "Config.h"

namespace ArcJura
{
  /** The class for main JURA functionality. Traverses the 'logs' dir
   *  of the given control directory, and reports usage data extracted from 
   *  job log files within.
   */
  class UsageReporter:public Reporter
  {
  private:
    Arc::Logger logger;
    Destinations *dests;
    /** Directory where A-REX puts job logs */
    std::string job_log_dir;
    time_t expiration_time;
    std::vector<std::string> urls; 
    std::vector<std::string> topics;
    std::string out_dir;
    ArcJura::Config config;
  public:
    /** Constructor. Gets the job log dir and the expiration time in seconds.
     *  Default expiration time is infinity (represented by zero value).
     */
    UsageReporter(ArcJura::Config config);
    /** Processes job log files in '<control_dir>/logs'. */
    int report();
    ~UsageReporter();
  };

}

#endif
