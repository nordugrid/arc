// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>

#include "SubmitterARC1.h"
#include "AREXClient.h"

namespace Arc {

  Logger SubmitterARC1::logger(Submitter::logger, "ARC1");

  SubmitterARC1::SubmitterARC1(const UserConfig& usercfg)
    : Submitter(usercfg, "ARC1") {}

  SubmitterARC1::~SubmitterARC1() {}

  Plugin* SubmitterARC1::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg)
      return NULL;
    return new SubmitterARC1(*subarg);
  }

  URL SubmitterARC1::Submit(const JobDescription& jobdesc,
                            const ExecutionTarget& et) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(et.url, cfg, usercfg.Timeout());

    JobDescription job(jobdesc);

    if (!ModifyJobDescription(job, et)) {
      logger.msg(ERROR, "Submit: Failed to modify job description "
                        "to be sent to target.");
      return URL();
    }

    std::string jobid;
    if (!ac.submit(job.UnParse("ARCJSDL"), jobid, et.url.Protocol() == "https")) {
      logger.msg(ERROR, "Failed submitting job");
      return URL();
    }
    if (jobid.empty()) {
      logger.msg(ERROR, "Service returned no job identifier");
      return URL();
    }

    XMLNode jobidx(jobid);
    URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(job, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return URL();
    }

    AddJob(job, session_url, et.Cluster, session_url);

    return session_url;
  }

  URL SubmitterARC1::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             const ExecutionTarget& et,
                             bool forcemigration) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    AREXClient ac(et.url, cfg, usercfg.Timeout());

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

    if (!ModifyJobDescription(job, et)) {
      logger.msg(ERROR, "Submit: Failed to modify job description "
                        "to be sent to target.");
      return URL();
    }

    // Add ActivityOldId.
    job.Identification.ActivityOldId.push_back(jobid.str());

    std::string newjobid;
    if (!ac.migrate(idstr, job.UnParse("ARCJSDL"), forcemigration, newjobid,
                    et.url.Protocol() == "https")) {
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

    AddJob(job, session_url, et.Cluster, session_url);

    return session_url;
  }

  bool SubmitterARC1::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    // Check for identical file names.
    for (std::list<FileType>::const_iterator it1 = jobdesc.DataStaging.File.begin();
         it1 != jobdesc.DataStaging.File.end(); it1++) {
      for (std::list<FileType>::const_iterator it2 = it1;
           it2 != jobdesc.DataStaging.File.end(); it2++) {
        if (it1 == it2) continue;

        if (it1->Name == it2->Name && (!it1->Source.empty() && !it2->Source.empty() ||
                                       !it1->Target.empty() && !it2->Target.empty())) {
          logger.msg(DEBUG, "Two files have identical file name '%s'.", it1->Name);
          return false;
        }
      }
    }

    if (!jobdesc.Resources.RunTimeEnvironment.empty() &&
        !jobdesc.Resources.RunTimeEnvironment.selectSoftware(et.ApplicationEnvironments)) {
      // This error should never happen since RTE is checked in the Broker.
      logger.msg(DEBUG, "Unable to select run time environment");
      return false;
    }

    if (!jobdesc.Resources.CEType.empty() &&
        !jobdesc.Resources.CEType.selectSoftware(et.Implementation)) {
      // This error should never happen since Middleware is checked in the Broker.
      logger.msg(DEBUG, "Unable to select middleware");
      return false;
    }

    if (!jobdesc.Resources.OperatingSystem.empty() &&
        !jobdesc.Resources.OperatingSystem.selectSoftware(et.Implementation)) {
      // This error should never happen since OS is checked in the Broker.
      logger.msg(DEBUG, "Unable to select operating system.");
      return false;
    }

    return true;
  }

} // namespace Arc
