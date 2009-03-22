// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JSDLPARSER_H__
#define __ARC_JSDLPARSER_H__

#include <string>

#include "JobDescriptionParser.h"

namespace Arc {

  class JSDLParser
    : public JobDescriptionParser {
  public:
    JSDLParser();
    ~JSDLParser();
    JobDescription Parse(const std::string& source) const;
    std::string UnParse(const JobDescription& job) const;
  };

} // namespace Arc

#endif // __ARC_JSDLPARSER_H__
