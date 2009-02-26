#ifndef _AREXJARM_H
#define _AREXJARM_H

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
#define AREXJARM_DEFAULT_CONFIG_FILE     "/etc/arexjarm.xml"
#define AREXJARM_DEFAULT_JOBLOG_DIR      "/tmp/jobstatus/logs"
#define AREXJARM_DEFAULT_MAX_UR_SET_SIZE 50 //just like in original JARM

#endif
