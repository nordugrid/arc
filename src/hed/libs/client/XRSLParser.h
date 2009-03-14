#ifndef __ARC_XRSLPARSER_H__
#define __ARC_XRSLPARSER_H__

#include <string>
#include <arc/client/JobInnerRepresentation.h>

#include "JobDescriptionParser.h"

namespace Arc {

  class RSL;
  class RSLCondition;

  class XRSLParser
    : public JobDescriptionParser {
  public:
    bool parse(JobInnerRepresentation& j, const std::string source);
    bool getProduct(const JobInnerRepresentation& j,
		    std::string& product) const;
  private:
    bool parse(const RSL* r, JobInnerRepresentation& j);

    static bool SingleValue(const RSLCondition* c,
			    std::string& value);
    static bool ListValue(const RSLCondition* c,
			  std::list<std::string>& value);
    static bool SeqListValue(const RSLCondition* c,
			     std::list<std::list<std::string> >& value);
  };

} // namespace Arc

#endif // __ARC_XRSLPARSER_H__
