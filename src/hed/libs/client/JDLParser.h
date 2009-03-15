// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JDLPARSER_H__
#define __ARC_JDLPARSER_H__

#include <string>
#include <vector>
#include <arc/client/JobInnerRepresentation.h>

#include "JobDescriptionParser.h"


namespace Arc {

  class JDLParser
    : public JobDescriptionParser {
  private:
    bool splitJDL(std::string original_string, std::vector<std::string>& lines) const;
    bool handleJDLattribute(std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation) const;
    std::string simpleJDLvalue(std::string attributeValue) const;
    std::vector<std::string> listJDLvalue(std::string attributeValue) const;
  public:
    bool parse(Arc::JobInnerRepresentation& innerRepresentation, const std::string source);
    bool getProduct(const Arc::JobInnerRepresentation& innerRepresentation, std::string& product) const;
  };

} // namespace Arc

#endif // __ARC_JDLPARSER_H__
