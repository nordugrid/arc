// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ADLPARSER_H__
#define __ARC_ADLPARSER_H__

#include <string>

#include <arc/compute/JobDescriptionParserPlugin.h>

/** ADLParser
 * The ADLParser class, derived from the JobDescriptionParserPlugin class, is
 * a job description parser for the EMI ES job description language (ADL)
 * described in <http://>.
 */

namespace Arc {

  template<typename T> class Range;
  class Software;
  class SoftwareRequirement;

  class ADLParser
    : public JobDescriptionParserPlugin {
  public:
    ADLParser(PluginArgument* parg);
    ~ADLParser();
    JobDescriptionParserPluginResult Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "") const;
    JobDescriptionParserPluginResult Assemble(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect = "") const;

    static Plugin* Instance(PluginArgument *arg);
  };

} // namespace Arc

#endif // __ARC_ADLPARSER_H__
