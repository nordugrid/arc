#ifndef _USAGEPUBLISHER_H
#define _USAGEPUBLISHER_H

#include <string>

#include <arc/Logger.h>
#include <arc/ArcConfig.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "LUTSClient.h"

namespace Arc
{
  
  class UsagePublisher
  {
  private:
    Arc::Config &config;
    Arc::Logger &logger;
    Arc::LUTSClient lutsclient;

    /** Directory where A-REX puts job logs */
    std::string arex_joblog_dir;
    /** Max number of URs to put in a set before submitting it */
    int max_ur_set_size;

    /** Usage Record set XML */
    Arc::XMLNode usagerecordset;
    /** Actual number of usage records in set */
    int urn;
    /** Job ID to logfile hash list mapping */
    jobmap_t jobs;
    /** List of job IDs the logs of which have been successfully 
	logged, and can be be deleted. */
    joblist_t deletable_jobs;

    int log_and_delete();
  public:
    UsagePublisher(Config &_config, Logger &_logger);
    int publish();
  };

}

#endif
