#ifndef _AREXLUTSCLIENT_H
#define _AREXLUTSCLIENT_H

#include <string>
#include <map>
#include <list>


typedef std::list<std::string> hashlist_t;

/**
 * Jobnumber to hash list mapping for registering 
 * what logs a job has.
 */
typedef std::map<std::string, hashlist_t> jobmap_t;

/**
 * List of jobnumbers, to register jobs that have been
 * successfully logged, and the logs of which can be deleted.
 */
typedef std::list<std::string> joblist_t;

//Default values for configuration:
//#define AREXLUTSCLIENT_DEFAULT_CONFIG_FILE        "/etc/arexlutsclient.xml"
#define AREXLUTSCLIENT_DEFAULT_JOBLOG_DIR         "/tmp/jobstatus/logs"
#define AREXLUTSCLIENT_DEFAULT_MAX_UR_SET_SIZE    50 //just like in original JARM
#define AREXLUTSCLIENT_DEFAULT_LOG_FILE           "/tmp/arexlutsclient.log"
#define AREXLUTSCLIENT_DEFAULT_REPORTING_INTERVAL 3600

#endif
