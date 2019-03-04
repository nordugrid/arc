#ifndef _REREPORTER_H
#define _REREPORTER_H

#include <time.h>

#include <vector>
#include <string>

#include <arc/Logger.h>

#include "Reporter.h"
#include "Destination.h"

namespace ArcJura
{
  /** The class for main JURA functionality. Traverses the 'logs' dir
   *  of the given control directory, and reports usage data extracted from 
   *  job log files within.
   */
  class ReReporter:public Reporter
  {
  private:
    Arc::Logger logger;
    Destination *dest;
    /** Directory where A-REX puts archived job logs */
    std::string archivedjob_log_dir;
    struct tm* start;
    struct tm* end;
    std::vector<std::string> urls; 
    std::vector<std::string> topics;
    std::string vo_filters;
    std::string regexp;
  public:
    /** Constructor. Gets the job log dir and the expiration time in seconds.
     *  Default expiration time is infinity (represented by zero value).
     */
    ReReporter(std::string archivedjob_log_dir_, std::string time_range_="",
                  std::vector<std::string> urls_=std::vector<std::string>(),
                  std::vector<std::string> topics_=std::vector<std::string>(),
                  std::string vo_filters_="");
    /** Processes job log files in '<control_dir>/logs'. */
    int report();
    ~ReReporter();
  };

}

#endif
