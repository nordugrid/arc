// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JDLPARSER_H__
#define __ARC_JDLPARSER_H__

#include <list>
#include <string>
#include <vector>

#include "JobDescriptionParser.h"

namespace Arc {

  class JDLParser
    : public JobDescriptionParser {
  public:
    JDLParser();
    ~JDLParser();
    JobDescription Parse(const std::string& source) const;
    std::string UnParse(const JobDescription& job) const;
  private:
    bool splitJDL(const std::string& original_string,
                  std::vector<std::string>& lines) const;
    bool handleJDLattribute(const std::string& attributeName,
                            const std::string& attributeValue,
                            JobDescription& job) const;
    std::string simpleJDLvalue(const std::string& attributeValue) const;
    std::list<std::string> listJDLvalue(const std::string&
                                        attributeValue) const;
  };

} // namespace Arc

#endif // __ARC_JDLPARSER_H__
