#ifndef _USAGEREPORTER_H
#define _USAGEREPORTER_H

#include <time.h>

#include <string>

#include <arc/Logger.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "Destinations.h"

namespace Arc
{
  
  class UsageReporter
  {
  private:
    Arc::Logger logger;
    Arc::Destinations *dests;
    /** Directory where A-REX puts job logs */
    std::string job_log_dir;
    time_t expiration_time;
  public:
    UsageReporter(std::string job_log_dir_, time_t expiration_time_=0);
    int report();
    ~UsageReporter();
  };

}

#endif
