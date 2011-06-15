#ifndef _USAGEREPORTER_H
#define _USAGEREPORTER_H

#include <time.h>

#include <vector>
#include <string>

#include <arc/Logger.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "Destinations.h"

namespace Arc
{
  /** The class for main JURA functionality. Traverses the 'logs' dir
   *  of the given control directory, and reports usage data extracted from 
   *  job log files within.
   */
  class UsageReporter
  {
  private:
    Arc::Logger logger;
    Arc::Destinations *dests;
    /** Directory where A-REX puts job logs */
    std::string job_log_dir;
    time_t expiration_time;
    std::vector<std::string> urls; 
    std::vector<std::string> topics;
    std::string out_dir;
  public:
    /** Constructor. Gets the job log dir and the expiration time in seconds.
     *  Default expiration time is infinity (represented by zero value).
     */
    UsageReporter(std::string job_log_dir_, time_t expiration_time_=0,
                  std::vector<std::string> urls_=std::vector<std::string>(),
                  std::vector<std::string> topics_=std::vector<std::string>(),
                  std::string out_dir_="");
    /** Processes job log files in '<control_dir>/logs'. */
    int report();
    ~UsageReporter();
  };

}

#endif
