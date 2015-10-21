#ifndef __ARC_JOBDESCRIPTIONPARSERTESTACC_H__
#define __ARC_JOBDESCRIPTIONPARSERTESTACC_H__

#include <arc/compute/JobDescription.h>
#include <arc/compute/JobDescriptionParserPlugin.h>
#include <arc/compute/TestACCControl.h>

namespace Arc {

class JobDescriptionParserPluginTestACC
  : public JobDescriptionParserPlugin {
private:
  JobDescriptionParserPluginTestACC(PluginArgument* parg)
    : JobDescriptionParserPlugin(parg) {}

public:
  ~JobDescriptionParserPluginTestACC() {}

  static Plugin* GetInstance(PluginArgument *arg);

  virtual JobDescriptionParserPluginResult Parse(const std::string& /*source*/, std::list<JobDescription>& jobdescs, const std::string& /*language = ""*/, const std::string& /*dialect = ""*/) const { jobdescs = JobDescriptionParserPluginTestACCControl::parsedJobDescriptions; return JobDescriptionParserPluginTestACCControl::parseStatus; }
  virtual JobDescriptionParserPluginResult Assemble(const JobDescription& /*job*/, std::string& output, const std::string& /*language*/, const std::string& /*dialect = ""*/) const { output = JobDescriptionParserPluginTestACCControl::unparsedString; return JobDescriptionParserPluginTestACCControl::unparseStatus; }
};

}

#endif // __ARC_JOBDESCRIPTIONPARSERTESTACC_H__
