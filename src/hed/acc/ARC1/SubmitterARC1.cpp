// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>

#include "SubmitterARC1.h"
#include "AREXClient.h"

namespace Arc {

  Logger SubmitterARC1::logger(Submitter::logger, "ARC1");

  SubmitterARC1::SubmitterARC1(Config *cfg)
    : Submitter(cfg, "ARC1") {}

  SubmitterARC1::~SubmitterARC1() {}

  Plugin* SubmitterARC1::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new SubmitterARC1((Config*)(*accarg));
  }

  bool SubmitterARC1::Submit(const JobDescription& jobdesc, XMLNode& info) const {

    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(submissionEndpoint, cfg);

    std::string jobdescstring = jobdesc.UnParse("POSIXJSDL");
    std::istringstream jsdlfile(jobdescstring);

    std::string jobid;
    if (!ac.submit(jsdlfile, jobid, submissionEndpoint.Protocol() == "https")) {
      logger.msg(ERROR, "Failed submitting job");
      return false;
    }
    if (jobid.empty()) {
      logger.msg(ERROR, "Service returned no job identifier");
      return false;
    }

    XMLNode jobidx(jobid);
    URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(jobdesc, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return false;
    }

    info.NewChild("JobID") = session_url.str();
    info.NewChild("InfoEndpoint") = session_url.str();

    return true;
  }

  bool SubmitterARC1::Migrate(const URL& jobid, const JobDescription& jobdesc, bool forcemigration, XMLNode& info) const {

    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(submissionEndpoint, cfg);

    std::string idstr;
    AREXClient::createActivityIdentifier(jobid, idstr);
    
    std::string jobdescstring = jobdesc.UnParse("POSIXJSDL");

    std::string newjobid;
    if (!ac.migrate(idstr, jobdescstring, forcemigration, newjobid, submissionEndpoint.Protocol() == "https")) {
      logger.msg(ERROR, "Failed migrating job");
      return false;
    }
    if (newjobid.empty()) {
      logger.msg(ERROR, "Service returned no job identifier");
      return false;
    }

    XMLNode newjobidx(newjobid);
    URL session_url((std::string)(newjobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(jobdesc, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return false;
    }

    info.NewChild("JobID") = session_url.str();
    info.NewChild("InfoEndpoint") = session_url.str();

    return true;
  }

} // namespace Arc
