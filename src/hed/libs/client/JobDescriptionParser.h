// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBDESCRIPTIONPARSER_H__
#define __ARC_JOBDESCRIPTIONPARSER_H__

#include <string>

#include <arc/client/JobDescription.h>

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
