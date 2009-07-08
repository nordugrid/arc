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
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTIONPARSER_H__
