#ifndef JOBLOGFILE_H
#define JOBLOGFILE_H


#include <time.h>
#include <stdexcept>
#include <fstream>
#include <string>
#include <map>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>

namespace ArcJura
{
  class JobLogFile: public std::map<std::string, std::string>
  /**
   * Class to represent a job log file created by A-REX, 
   * and to create OGF Job Usage Records from them.
   */
  {
    std::string filename;
    bool allow_remove;
    std::vector<std::string> inputfiles;
    std::vector<std::string> outputfiles;
    std::string getArchivingPath(bool car=false);
    void parseInputOutputFiles(Arc::XMLNode &node, std::vector<std::string> &list, std::string type="input");
  public:
    /** Constructor. Loads and parses A-REX job log. */
    JobLogFile(const std::string& _filename):allow_remove(true) { parse(_filename); } 
    /** Reloads and parses A-REX job log. */
    int parse(const std::string& _filename);
    /** Write job log to another file */
    int write(const std::string& _filename);
    /** Creates an OGF Job Usage Record from parsed log files. 
     *  - Missing UR properties:
     *    -# ProcessID: Local PID(s) of job. Extraction is LRMS-specific and \n
     *       may not always be possible
     *    -# Charge: Amount of money or abstract credits charged for the job.
     *    -# Some differentiated properties e.g. network, disk etc.
     */
    void createUsageRecord(Arc::XMLNode &usagerecord,
                              const char *recordid_prefix="ur-");
    /** Creates an OGF 2.0 (CAR) Job Usage Record from parsed log files. */
    void createCARUsageRecord(Arc::XMLNode &usagerecord,
                              const char *recordid_prefix="ur-");
    /** Returns original full path to log file */
    std::string getFilename() { return filename; }
    /** Enables/disables file removal from disk */
    void allowRemove(bool a) { allow_remove=a; }
    /** Checks if file exists on the disk */
    bool exists();
    /** Checks if file was modified earlier than 'age' seconds ago */
    bool olderThan(time_t age);
    /** Deletes file from the disk */
    void remove();
  };
}

#endif
