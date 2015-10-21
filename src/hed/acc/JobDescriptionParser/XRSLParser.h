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
    virtual ~XRSLParser() {};
    virtual JobDescriptionParserPluginResult Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "") const;
    virtual JobDescriptionParserPluginResult Assemble(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect = "") const;

    static Plugin* Instance(PluginArgument *arg);

  private:
    void Parse(const RSL *r, JobDescription& job, const std::string& dialect, JobDescriptionParserPluginResult& result) const;
    static void SingleValue(const RSLCondition *c, std::string& value, JobDescriptionParserPluginResult& result);
    static void ListValue(const RSLCondition *c, std::list<std::string>& value, JobDescriptionParserPluginResult& result);
    static void SeqListValue(const RSLCondition *c,
                             std::list<std::list<std::string> >& value, JobDescriptionParserPluginResult& result,
                             int seqlength = -1);
    static void ParseExecutablesAttribute(JobDescription& j, JobDescriptionParserPluginResult& result);
    static void ParseFTPThreadsAttribute(JobDescription& j, JobDescriptionParserPluginResult& result);
    static void ParseCacheAttribute(JobDescription& j, JobDescriptionParserPluginResult& result);
    static void ParseJoinAttribute(JobDescription& j, JobDescriptionParserPluginResult& result);
    static void ParseGridTimeAttribute(JobDescription& j, JobDescriptionParserPluginResult& result);
    static void ParseCountPerNodeAttribute(JobDescription& j, JobDescriptionParserPluginResult& result);
  };

} // namespace Arc

#endif // __ARC_XRSLPARSER_H__
