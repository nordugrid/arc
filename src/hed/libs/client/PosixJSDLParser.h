// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_POSIXJSDLPARSER_H__
#define __ARC_POSIXJSDLPARSER_H__

#include <string>

#include "JobDescriptionParser.h"

namespace Arc {

  class PosixJSDLParser
    : public JobDescriptionParser {
  public:
    PosixJSDLParser();
    ~PosixJSDLParser();
    JobDescription Parse(const std::string& source) const;
    std::string UnParse(const JobDescription& job) const;
  };

} // namespace Arc

#endif // __ARC_POSIXJSDLPARSER_H__
