// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <arc/client/JobDescription.h>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>

#include "SubmitterARC1.h"
#include "AREXClient.h"

namespace Arc {

  Logger SubmitterARC1::logger(Submitter::logger, "ARC1");

  SubmitterARC1::SubmitterARC1(const Config& cfg, const UserConfig& usercfg)
    : Submitter(cfg, usercfg, "ARC1") {}

  SubmitterARC1::~SubmitterARC1() {}

  Plugin* SubmitterARC1::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg)
      return NULL;
    return new SubmitterARC1(*subarg, *subarg);
  }

  URL SubmitterARC1::Submit(const JobDescription& jobdesc,
                            const std::string& joblistfile) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
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

    JobDescription job(jobdesc);

    if (!PutFiles(job, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return URL();
    }

    AddJob(job, session_url, session_url, joblistfile);

    return session_url;
  }

  URL SubmitterARC1::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             bool forcemigration,
                             const std::string& joblistfile) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(submissionEndpoint, cfg);

    std::string idstr;
    AREXClient::createActivityIdentifier(jobid, idstr);

    JobDescription job(jobdesc);

    // Modify the location of local files and files residing in a old session directory.
    for (std::list<FileType>::iterator it = job.DataStaging.File.begin();
         it != job.DataStaging.File.end(); it++) {
      // Do not modify Output and Error files.
      if (it->Name == job.Application.Output ||
          it->Name == job.Application.Error ||
          it->Source.empty())
        continue;

      if (!it->Source.front().URI || it->Source.front().URI.Protocol() == "file") {
        it->Source.front().URI = URL(jobid.str() + "/" + it->Name);
        it->DownloadToCache = false;
      }
      else {
        // URL is valid, and not a local file. Check if the source reside at a
        // old job session directory.
        const size_t foundRSlash = it->Source.front().URI.str().rfind('/');
        if (foundRSlash == std::string::npos)
          continue;

        const std::string uriPath = it->Source.front().URI.str().substr(0, foundRSlash);
        // Check if the input file URI is pointing to a old job session directory.
        for (std::list<std::string>::const_iterator itAOID = job.Identification.ActivityOldId.begin();
             itAOID != job.Identification.ActivityOldId.end(); itAOID++)
          if (uriPath == *itAOID) {
            it->Source.front().URI = URL(jobid.str() + "/" + it->Name);
            it->DownloadToCache = false;
            break;
          }
      }
    }

    // Add ActivityOldId.
    job.Identification.ActivityOldId.push_back(jobid.str());

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

    if (!PutFiles(job, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return URL();
    }

    AddJob(job, session_url, session_url, joblistfile);

    return session_url;
  }

} // namespace Arc
