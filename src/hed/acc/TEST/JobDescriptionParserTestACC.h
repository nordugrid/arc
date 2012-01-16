#ifndef __ARC_JOBDESCRIPTIONPARSERTESTACC_H__
#define __ARC_JOBDESCRIPTIONPARSERTESTACC_H__

#include <arc/client/JobDescription.h>
#include <arc/client/JobDescriptionParser.h>
#include <arc/client/TestACCControl.h>

namespace Arc {

class JobDescriptionParserTestACC
  : public JobDescriptionParser {
private:
  JobDescriptionParserTestACC()
    : JobDescriptionParser() {}

public:
  ~JobDescriptionParserTestACC() {}

  static Plugin* GetInstance(PluginArgument *arg);

  virtual JobDescriptionParserResult Parse(const std::string& /*source*/, std::list<JobDescription>& jobdescs, const std::string& /*language = ""*/, const std::string& /*dialect = ""*/) const { if (JobDescriptionParserTestACCControl::parsedJobDescriptions) jobdescs = *JobDescriptionParserTestACCControl::parsedJobDescriptions; return JobDescriptionParserTestACCControl::parseStatus; }
  virtual JobDescriptionParserResult UnParse(const JobDescription& /*job*/, std::string& output, const std::string& /*language*/, const std::string& /*dialect = ""*/) const { if (JobDescriptionParserTestACCControl::unparsedString) output = *JobDescriptionParserTestACCControl::unparsedString; return JobDescriptionParserTestACCControl::unparseStatus; }
};

}

#endif // __ARC_JOBDESCRIPTIONPARSERTESTACC_H__
