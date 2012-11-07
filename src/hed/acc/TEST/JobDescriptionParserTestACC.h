#ifndef __ARC_JOBDESCRIPTIONPARSERTESTACC_H__
#define __ARC_JOBDESCRIPTIONPARSERTESTACC_H__

#include <arc/compute/JobDescription.h>
#include <arc/compute/JobDescriptionParser.h>
#include <arc/compute/TestACCControl.h>

namespace Arc {

class JobDescriptionParserTestACC
  : public JobDescriptionParser {
private:
  JobDescriptionParserTestACC(PluginArgument* parg)
    : JobDescriptionParser(parg) {}

public:
  ~JobDescriptionParserTestACC() {}

  static Plugin* GetInstance(PluginArgument *arg);

  virtual JobDescriptionParserResult Parse(const std::string& /*source*/, std::list<JobDescription>& jobdescs, const std::string& /*language = ""*/, const std::string& /*dialect = ""*/) const { jobdescs = JobDescriptionParserTestACCControl::parsedJobDescriptions; return JobDescriptionParserTestACCControl::parseStatus; }
  virtual JobDescriptionParserResult UnParse(const JobDescription& /*job*/, std::string& output, const std::string& /*language*/, const std::string& /*dialect = ""*/) const { output = JobDescriptionParserTestACCControl::unparsedString; return JobDescriptionParserTestACCControl::unparseStatus; }
};

}

#endif // __ARC_JOBDESCRIPTIONPARSERTESTACC_H__
