// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBDESCRIPTIONPARSER_H__
#define __ARC_JOBDESCRIPTIONPARSER_H__

#include <string>

#include <arc/client/JobDescription.h>

/** JobDescriptionParser
 * The JobDescriptionParser class is abstract which provide a interface for job
 * description parsers. A job description parser should inherit this class and
 * overwrite the JobDescriptionParser::Parse and
 * JobDescriptionParser::UnParse methods.
 */

namespace Arc {

  class Logger;

  // Abstract class for the different parsers
  class JobDescriptionParser {
  public:
    JobDescriptionParser();
    virtual ~JobDescriptionParser();
    virtual JobDescription Parse(const std::string& source) const = 0;
    virtual std::string UnParse(const JobDescription& job) const = 0;
    void AddHint(const std::string& key,const std::string& value);
    void SetHints(const std::map<std::string,std::string>& hints);
  protected:
    static Logger logger;
    std::map<std::string,std::string> hints;
    std::string GetHint(const std::string& key) const;
  };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTIONPARSER_H__
