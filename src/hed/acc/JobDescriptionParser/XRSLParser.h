// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_XRSLPARSER_H__
#define __ARC_XRSLPARSER_H__

#include <list>
#include <string>

#include <arc/client/JobDescriptionParser.h>

/** XRSLParser
 * The XRSLParser class, derived from the JobDescriptionParser class, is a
 * job description parser for the Extended Resource Specification Language
 * (XRSL) specified in the NORDUGRID-MANUAL-4 document.
 */

namespace Arc {

  class RSL;
  class RSLCondition;

  class XRSLParser
    : public JobDescriptionParser {
  public:
    XRSLParser();
    ~XRSLParser();
    bool Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "") const;
    bool UnParse(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect = "") const;

    static Plugin* Instance(PluginArgument *arg);

  private:
    bool Parse(const RSL *r, JobDescription& job, const std::string& dialect) const;
    static bool SingleValue(const RSLCondition *c,
                            std::string& value);
    static bool ListValue(const RSLCondition *c,
                          std::list<std::string>& value);
    static bool SeqListValue(const RSLCondition *c,
                             std::list<std::list<std::string> >& value,
                             int seqlength = -1);
    static bool cached;
  };

} // namespace Arc

#endif // __ARC_XRSLPARSER_H__
