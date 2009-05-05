#ifndef JOBLOGFILE_H
#define JOBLOGFILE_H


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
   * Class to parse logs created by A-REX for jobs, 
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
     *    -# Host: List of identifiers of hosts the job ran on.
     *    -# ProjectName: The identifier of the entity (project) to charge.\n
     *       Job submission descriptor may contain this value, or should be\n
     *       deduced from user identifier info (via some VO-project mapping?)
     *    -# Differentiated properties
     */
    void createUsageRecord(Arc::XMLNode &usagerecord,
			      const char *recordid_prefix="ur-");
    /** Returns original full path to log file */
    std::string getFilename() { return filename; }
    bool exists();
    void remove();
  };
}

#endif
