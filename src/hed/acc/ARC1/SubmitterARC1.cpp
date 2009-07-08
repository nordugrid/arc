// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <arc/FileLock.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Sandbox.h>
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

  URL SubmitterARC1::Submit(const JobDescription& jobdesc,
                            const std::string& joblistfile) const {
    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(submissionEndpoint, cfg);

    std::string jobdescstring = jobdesc.UnParse("ARCJSDL");
    std::istringstream jsdlfile(jobdescstring);

    std::string jobid;
    if (!ac.submit(jsdlfile, jobid, submissionEndpoint.Protocol() == "https")) {
      logger.msg(ERROR, "Failed submitting job");
      return URL();
    }
    if (jobid.empty()) {
      logger.msg(ERROR, "Service returned no job identifier");
      return URL();
    }

    XMLNode jobidx(jobid);
    URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(jobdesc, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return URL();
    }

    Arc::NS ns;
    Arc::XMLNode info(ns, "Job");
    info.NewChild("JobID") = session_url.str();
    if (!jobdesc.Identification.JobName.empty())
      info.NewChild("Name") = jobdesc.Identification.JobName;
    info.NewChild("Flavour") = flavour;
    info.NewChild("Cluster") = cluster.str();
    info.NewChild("InfoEndpoint") = session_url.str();
    info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();
    Sandbox::Add(jobdesc, info);

    FileLock lock(joblistfile);
    Config jobs;
    jobs.ReadFromFile(joblistfile);
    jobs.NewChild(info);
    jobs.SaveToFile(joblistfile);

    return session_url;
  }

  URL SubmitterARC1::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             bool forcemigration,
                             const std::string& joblistfile) const {
    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(submissionEndpoint, cfg);

    std::string idstr;
    AREXClient::createActivityIdentifier(jobid, idstr);
    
    std::string jobdescstring = jobdesc.UnParse("ARCJSDL");

    std::string newjobid;
    if (!ac.migrate(idstr, jobdescstring, forcemigration, newjobid,
                    submissionEndpoint.Protocol() == "https")) {
      logger.msg(ERROR, "Failed migrating job");
      return URL();
    }
    if (newjobid.empty()) {
      logger.msg(ERROR, "Service returned no job identifier");
      return URL();
    }

    XMLNode newjobidx(newjobid);
    URL session_url((std::string)(newjobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(jobdesc, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return URL();
    }

    Arc::NS ns;
    Arc::XMLNode info(ns, "Job");
    info.NewChild("JobID") = session_url.str();
    if (!jobdesc.Identification.JobName.empty())
      info.NewChild("Name") = jobdesc.Identification.JobName;
    info.NewChild("Flavour") = flavour;
    info.NewChild("Cluster") = cluster.str();
    info.NewChild("InfoEndpoint") = session_url.str();
    info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();
    Sandbox::Add(jobdesc, info);

    FileLock lock(joblistfile);
    Config jobs;
    jobs.ReadFromFile(joblistfile);
    jobs.NewChild(info);
    jobs.SaveToFile(joblistfile);

    return session_url;
  }

} // namespace Arc
