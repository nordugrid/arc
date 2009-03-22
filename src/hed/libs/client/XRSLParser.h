// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_XRSLPARSER_H__
#define __ARC_XRSLPARSER_H__

#include <list>
#include <string>

#include "JobDescriptionParser.h"

namespace Arc {

  class RSL;
  class RSLCondition;

  class XRSLParser
    : public JobDescriptionParser {
  public:
    XRSLParser();
    ~XRSLParser();
    JobDescription Parse(const std::string& source) const;
    std::string UnParse(const JobDescription& job) const;
  private:
    bool Parse(const RSL *r, JobDescription& job) const;
    static bool SingleValue(const RSLCondition *c,
                            std::string& value);
    static bool ListValue(const RSLCondition *c,
                          std::list<std::string>& value);
    static bool SeqListValue(const RSLCondition *c,
                             std::list<std::list<std::string> >& value,
                             int seqlength = -1);
  };

} // namespace Arc

#endif // __ARC_XRSLPARSER_H__
