#ifndef JOBLOGFILE_H
#define JOBLOGFILE_H


#include <time.h>
#include <stdexcept>
#include <fstream>
#include <string>
#include <map>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>

namespace Arc
{
  class JobLogFile: public std::map<std::string, std::string>
  /**
   * Class to represent a job log file created by A-REX, 
   * and to create OGF Job Usage Records from them.
   */
  {
    std::string filename;
  public:
    /** Constructor. Loads and parses A-REX job log. */
    JobLogFile(const std::string& _filename) { parse(_filename); } 
    /** Reloads and parses A-REX job log. */
    int parse(const std::string& _filename);
    /** Creates an OGF Job Usage Record from parsed log files. 
     *  - Missing UR properties:
     *    -# ProcessID: Local PID(s) of job. Extraction is LRMS-specific and \n
     *       may not always be possible
     *    -# Charge: Amount of money or abstract credits charged for the job.
     *    -# Some differentiated properties e.g. network, disk etc.
     */
    void createUsageRecord(Arc::XMLNode &usagerecord,
			      const char *recordid_prefix="ur-");
    /** Returns original full path to log file */
    std::string getFilename() { return filename; }
    /** Checks if file exists on the disk */
    bool exists();
    /** Checks if file was modified earlier than 'age' seconds ago */
    bool olderThan(time_t age);
    /** Deletes file from the disk */
    void remove();
  };
}

#endif
