// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <sys/stat.h>

#include <glibmm.h>

#include <arc/CheckSum.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>

#include "SubmitterARC1.h"
#include "AREXClient.h"

namespace Arc {

  Logger SubmitterARC1::logger(Logger::getRootLogger(), "Submitter.ARC1");

  SubmitterARC1::SubmitterARC1(const UserConfig& usercfg, PluginArgument* parg)
    : Submitter(usercfg, "ARC1", parg) {
  }

  SubmitterARC1::~SubmitterARC1() {
    deleteAllClients();
  }

  Plugin* SubmitterARC1::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg) return NULL;
    return new SubmitterARC1(*subarg, arg);
  }

  AREXClient* SubmitterARC1::acquireClient(const URL& url) {
    std::map<URL, AREXClient*>::const_iterator url_it = clients.find(url);
    if ( url_it != clients.end() ) {
      // If AREXClient is already existing for the
      // given URL then return with that
      return url_it->second;
    } else {
      // Else create a new one and return with that
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      AREXClient* ac = new AREXClient(url, cfg, usercfg.Timeout());
      return clients[url] = ac;
    }
  }

  bool SubmitterARC1::releaseClient(const URL& url) {
    return true;
  }

  bool SubmitterARC1::deleteAllClients() {
    std::map<URL, AREXClient*>::iterator it;
    for (it = clients.begin(); it != clients.end(); it++) {
        if ((*it).second != NULL) delete (*it).second;
    }
    return true;
  }

  bool SubmitterARC1::Submit(const JobDescription& jobdesc,
                             const ExecutionTarget& et, Job& job) {
    URL url(et.ComputingEndpoint->URLString);

    AREXClient* ac = acquireClient(url);

    JobDescription preparedjobdesc(jobdesc);

    if (!preparedjobdesc.Prepare(et)) {
      logger.msg(INFO, "Failed to prepare job description to target resources");
      releaseClient(url);
      return false;
    }

    std::string product;
    if (!preparedjobdesc.UnParse(product, "nordugrid:jsdl")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "nordugrid:jsdl");
      releaseClient(url);
      return false;
    }

    std::string sJobid;
    if (!ac->submit(product, sJobid, url.Protocol() == "https")) {
      releaseClient(url);
      return false;
    }

    if (sJobid.empty()) {
      logger.msg(INFO, "No job identifier returned by A-REX");
      releaseClient(url);
      return false;
    }

    XMLNode jobidx(sJobid);
    URL jobid((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(preparedjobdesc, jobid)) {
      logger.msg(INFO, "Failed uploading local input files");
      releaseClient(url);
      return false;
    }

    AddJobDetails(preparedjobdesc, jobid, et.ComputingService->Cluster, jobid, job);

    releaseClient(url);
    return true;
  }

  bool SubmitterARC1::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             const ExecutionTarget& et,
                             bool forcemigration, Job& job) {
    URL url(et.ComputingEndpoint->URLString);

    AREXClient* ac = acquireClient(url);

    std::string idstr;
    AREXClient::createActivityIdentifier(jobid, idstr);

    JobDescription preparedjobdesc(jobdesc);

    // Modify the location of local files and files residing in a old session directory.
    for (std::list<InputFileType>::iterator it = preparedjobdesc.DataStaging.InputFiles.begin();
         it != preparedjobdesc.DataStaging.InputFiles.end(); it++) {
      if (!it->Sources.front() || it->Sources.front().Protocol() == "file") {
        it->Sources.front() = URL(jobid.str() + "/" + it->Name);
      }
      else {
        // URL is valid, and not a local file. Check if the source reside at a
        // old job session directory.
        const size_t foundRSlash = it->Sources.front().str().rfind('/');
        if (foundRSlash == std::string::npos) continue;

        const std::string uriPath = it->Sources.front().str().substr(0, foundRSlash);
        // Check if the input file URI is pointing to a old job session directory.
        for (std::list<std::string>::const_iterator itAOID = preparedjobdesc.Identification.ActivityOldID.begin();
             itAOID != preparedjobdesc.Identification.ActivityOldID.end(); itAOID++) {
          if (uriPath == *itAOID) {
            it->Sources.front() = URL(jobid.str() + "/" + it->Name);
            break;
          }
        }
      }
    }

    if (!preparedjobdesc.Prepare(et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      releaseClient(url);
      return false;
    }

    // Add ActivityOldID.
    preparedjobdesc.Identification.ActivityOldID.push_back(jobid.str());

    std::string product;
    if (!preparedjobdesc.UnParse(product, "nordugrid:jsdl")) {
      logger.msg(INFO, "Unable to migrate job. Job description is not valid in the %s format", "nordugrid:jsdl");
      releaseClient(url);
      return false;
    }

    std::string sNewjobid;
    if (!ac->migrate(idstr, product, forcemigration, sNewjobid,
                    url.Protocol() == "https")) {
      releaseClient(url);
      return false;
    }

    if (sNewjobid.empty()) {
      logger.msg(INFO, "No job identifier returned by A-REX");
      releaseClient(url);
      return false;
    }

    XMLNode xNewjobid(sNewjobid);
    URL newjobid((std::string)(xNewjobid["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(preparedjobdesc, newjobid)) {
      logger.msg(INFO, "Failed uploading local input files");
      releaseClient(url);
      return false;
    }

    AddJobDetails(preparedjobdesc, newjobid, et.ComputingService->Cluster, newjobid, job);

    releaseClient(url);
    return true;
  }
} // namespace Arc
