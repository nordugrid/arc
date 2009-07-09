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

    std::string jobid;
    if (!ac.submit(jobdesc.UnParse("ARCJSDL"), jobid, submissionEndpoint.Protocol() == "https")) {
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

    JobDescription job(jobdesc);

    // Add ActivityOldId.
    job.Identification.ActivityOldId.push_back(jobid.str());

    // Modify the location of local files and files residing in a old session directory.
    for (std::list<FileType>::iterator it = job.DataStaging.File.begin();
         it != job.DataStaging.File.end(); it++) {
      // Do not modify Output and Error files.
      if (it->Name == job.Application.Output ||
          it->Name == job.Application.Error)
        continue;

      if (it->Source.size() == 0) {
        DataSourceType source;
        source.URI = URL(jobid.str() + "/" + it->Name);
        it->Source.push_back(source);
        it->DownloadToCache = false;
      }
      else if (!it->Source.front().URI) {
        it->Source.front().URI = URL(jobid.str() + "/" + it->Name);
        it->DownloadToCache = false;
      }
      else {
        const size_t foundRSlash = it->Source.front().URI.str().rfind('/');
        if (foundRSlash == std::string::npos)
          continue;
        
        const std::string uriPath = it->Source.front().URI.str().substr(0, foundRSlash);
        // Check if the input file URI is pointing to a old job session directory.
        for (std::list<std::string>::const_iterator itAOID = job.Identification.ActivityOldId.begin();
             itAOID != job.Identification.ActivityOldId.end(); itAOID++) {
          if (uriPath == *itAOID) {
            it->Source.front().URI = URL(jobid.str() + "/" + it->Name);
            it->DownloadToCache = false;
            break;
          }
        }
      }
    }

    std::string newjobid;
    if (!ac.migrate(idstr, job.UnParse("ARCJSDL"), forcemigration, newjobid,
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
