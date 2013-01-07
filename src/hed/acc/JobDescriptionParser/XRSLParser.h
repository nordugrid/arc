// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_XRSLPARSER_H__
#define __ARC_XRSLPARSER_H__

#include <list>
#include <string>

#include <arc/compute/JobDescriptionParserPlugin.h>

/** XRSLParser
 * The XRSLParser class, derived from the JobDescriptionParserPlugin class, is a
 * job description parser for the Extended Resource Specification Language
 * (XRSL) specified in the NORDUGRID-MANUAL-4 document.
 */

namespace Arc {

  class RSL;
  class RSLCondition;

  class XRSLParser
    : public JobDescriptionParserPlugin {
  public:
    XRSLParser(PluginArgument* parg);
    virtual ~XRSLParser();
    virtual JobDescriptionParserPluginResult Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "") const;
    virtual JobDescriptionParserPluginResult UnParse(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect = "") const;

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
    static bool ParseExecutablesAttribute(JobDescription& j);
    static bool ParseFTPThreadsAttribute(JobDescription& j);
    static bool ParseCacheAttribute(JobDescription& j);
    static bool ParseJoinAttribute(JobDescription& j);
    static bool ParseGridTimeAttribute(JobDescription& j);
    static bool ParseCountPerNodeAttribute(JobDescription& j);
  };

} // namespace Arc

#endif // __ARC_XRSLPARSER_H__
