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

  SubmitterARC1::SubmitterARC1(const UserConfig& usercfg)
    : Submitter(usercfg, "ARC1") {}

  SubmitterARC1::~SubmitterARC1() {
    deleteAllClients();
  }

  Plugin* SubmitterARC1::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg) return NULL;
    return new SubmitterARC1(*subarg);
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

    AREXClient* ac = acquireClient(et.url);

    JobDescription modjobdesc(jobdesc);

    if (!ModifyJobDescription(modjobdesc, et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      releaseClient(et.url);
      return false;
    }

    std::string product;
    if (!modjobdesc.UnParse(product, "nordugrid:jsdl")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "nordugrid:jsdl");
      releaseClient(et.url);
      return false;
    }

    std::string sJobid;
    if (!ac->submit(product, sJobid, et.url.Protocol() == "https")) {
      releaseClient(et.url);
      return false;
    }

    if (sJobid.empty()) {
      logger.msg(INFO, "No job identifier returned by A-REX");
      releaseClient(et.url);
      return false;
    }

    XMLNode jobidx(sJobid);
    URL jobid((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(modjobdesc, jobid)) {
      logger.msg(INFO, "Failed uploading local input files");
      releaseClient(et.url);
      return false;
    }

    AddJobDetails(modjobdesc, jobid, et.Cluster, jobid, job);

    releaseClient(et.url);
    return true;
  }

  bool SubmitterARC1::Migrate(const URL& jobid, const JobDescription& jobdesc,
                             const ExecutionTarget& et,
                             bool forcemigration, Job& job) {
    AREXClient* ac = acquireClient(et.url);

    std::string idstr;
    AREXClient::createActivityIdentifier(jobid, idstr);

    JobDescription modjobdesc(jobdesc);

    // Modify the location of local files and files residing in a old session directory.
    for (std::list<InputFileType>::iterator it = modjobdesc.DataStaging.InputFiles.begin();
         it != modjobdesc.DataStaging.InputFiles.end(); it++) {
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
        for (std::list<std::string>::const_iterator itAOID = modjobdesc.Identification.ActivityOldID.begin();
             itAOID != modjobdesc.Identification.ActivityOldID.end(); itAOID++) {
          if (uriPath == *itAOID) {
            it->Sources.front() = URL(jobid.str() + "/" + it->Name);
            break;
          }
        }
      }
    }

    if (!ModifyJobDescription(modjobdesc, et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      releaseClient(et.url);
      return false;
    }

    // Add ActivityOldID.
    modjobdesc.Identification.ActivityOldID.push_back(jobid.str());

    std::string product;
    if (!modjobdesc.UnParse(product, "nordugrid:jsdl")) {
      logger.msg(INFO, "Unable to migrate job. Job description is not valid in the %s format", "nordugrid:jsdl");
      releaseClient(et.url);
      return false;
    }

    std::string sNewjobid;
    if (!ac->migrate(idstr, product, forcemigration, sNewjobid,
                    et.url.Protocol() == "https")) {
      releaseClient(et.url);
      return false;
    }

    if (sNewjobid.empty()) {
      logger.msg(INFO, "No job identifier returned by A-REX");
      releaseClient(et.url);
      return false;
    }

    XMLNode xNewjobid(sNewjobid);
    URL newjobid((std::string)(xNewjobid["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(modjobdesc, newjobid)) {
      logger.msg(INFO, "Failed uploading local input files");
      releaseClient(et.url);
      return false;
    }

    AddJobDetails(modjobdesc, newjobid, et.Cluster, newjobid, job);

    releaseClient(et.url);
    return true;
  }

  bool SubmitterARC1::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    // Check for identical file names.
    bool executableIsAdded(false), inputIsAdded(false), outputIsAdded(false), errorIsAdded(false), logDirIsAdded(false);
    for (std::list<InputFileType>::iterator it1 = jobdesc.DataStaging.InputFiles.begin();
         it1 != jobdesc.DataStaging.InputFiles.end(); ++it1) {
      std::list<InputFileType>::const_iterator it2 = it1;
      for (++it2; it2 != jobdesc.DataStaging.InputFiles.end(); ++it2) {
        if (it1->Name == it2->Name && (!it1->Sources.empty() && !it2->Sources.empty())) {
          logger.msg(ERROR, "Two files have identical file name '%s'.", it1->Name);
          return false;
        }
      }

      if (!it1->Sources.empty() && it1->Sources.front().Protocol() == "file" && Glib::file_test(it1->Sources.front().Path(), Glib::FILE_TEST_EXISTS)) {
        logger.msg(ERROR, "Cannot stat local input file %s", it1->Sources.front().Path());
        return false;
      }

      executableIsAdded  |= (it1->Name == jobdesc.Application.Executable.Path);
      inputIsAdded       |= (it1->Name == jobdesc.Application.Input);
    }

    if (!executableIsAdded &&
        !Glib::path_is_absolute(jobdesc.Application.Executable.Path)) {
      jobdesc.DataStaging.InputFiles.push_back(InputFileType());
      InputFileType& file = jobdesc.DataStaging.InputFiles.back();
      file.Name = jobdesc.Application.Executable.Path;
      file.Sources.push_back(URL(file.Name));
      file.IsExecutable = true;
    }

    if (!jobdesc.Application.Input.empty() && !inputIsAdded) {
      jobdesc.DataStaging.InputFiles.push_back(InputFileType());
      InputFileType& file = jobdesc.DataStaging.InputFiles.back();
      file.Name = jobdesc.Application.Input;
      file.Sources.push_back(URL(file.Name));
      file.IsExecutable = false;
    }

    for (std::list<OutputFileType>::iterator it1 = jobdesc.DataStaging.OutputFiles.begin();
         it1 != jobdesc.DataStaging.OutputFiles.end(); ++it1) {
      outputIsAdded      |= (it1->Name == jobdesc.Application.Output);
      errorIsAdded       |= (it1->Name == jobdesc.Application.Error);
      logDirIsAdded      |= (it1->Name == jobdesc.Application.LogDir);
    }

    if (!jobdesc.Application.Output.empty() && !outputIsAdded) {
      jobdesc.DataStaging.OutputFiles.push_back(OutputFileType());
      OutputFileType& file = jobdesc.DataStaging.OutputFiles.back();
      file.Name = jobdesc.Application.Output;
    }

    if (!jobdesc.Application.Error.empty() && !errorIsAdded) {
      jobdesc.DataStaging.OutputFiles.push_back(OutputFileType());
      OutputFileType& file = jobdesc.DataStaging.OutputFiles.back();
      file.Name = jobdesc.Application.Error;
    }

    if (!jobdesc.Application.LogDir.empty() && !logDirIsAdded) {
      jobdesc.DataStaging.OutputFiles.push_back(OutputFileType());
      OutputFileType& file = jobdesc.DataStaging.OutputFiles.back();
      file.Name = jobdesc.Application.LogDir;
    }

    if (!jobdesc.Resources.RunTimeEnvironment.empty() &&
        !jobdesc.Resources.RunTimeEnvironment.selectSoftware(et.ApplicationEnvironments)) {
      // This error should never happen since RTE is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select run time environment");
      return false;
    }

    if (!jobdesc.Resources.CEType.empty() &&
        !jobdesc.Resources.CEType.selectSoftware(et.Implementation)) {
      // This error should never happen since Middleware is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select middleware");
      return false;
    }

    if (!jobdesc.Resources.OperatingSystem.empty() &&
        !jobdesc.Resources.OperatingSystem.selectSoftware(et.Implementation)) {
      // This error should never happen since OS is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select operating system.");
      return false;
    }

    // Set queue name to the selected ExecutionTarget
    jobdesc.Resources.QueueName = et.ComputingShareName;;

    return true;
  }

} // namespace Arc
