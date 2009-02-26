
#include <stdexcept>
#include <fstream>
#include <string>
#include <map>
#include <arc/XMLNode.h>
#include <arc/DateTime.h>

namespace Arc
{
  class ArexLogParser
  /**
   * Class to parse logs created by A-REX for jobs, 
   * and to create OGF Job Usage Records from them.
   */
  {
    std::map<std::string, std::string> joblog;
    
  public:
    /** Loads and parses A-REX job log. */
    int parse(const char *filename);
    /** Clears usage content. */
    void clear() {joblog.clear();}
    /** Creates an OGF Job Usage Record from parsed log files. */
    XMLNode createUsageRecord(const char *recordid_prefix="ur-");
  };
}
