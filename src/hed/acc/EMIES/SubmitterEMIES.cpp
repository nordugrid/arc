// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <sstream>

#include <glibmm.h>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>

#include "SubmitterEMIES.h"
#include "EMIESClient.h"

namespace Arc {

  Logger SubmitterEMIES::logger(Logger::getRootLogger(), "Submitter.EMIES");

  SubmitterEMIES::SubmitterEMIES(const UserConfig& usercfg)
    : Submitter(usercfg, "EMIES") {}

  SubmitterEMIES::~SubmitterEMIES() {
    deleteAllClients();
  }

  Plugin* SubmitterEMIES::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg) return NULL;
    return new SubmitterEMIES(*subarg);
  }

  EMIESClient* SubmitterEMIES::acquireClient(const URL& url) {
    std::map<URL, EMIESClient*>::const_iterator url_it = clients.find(url);
    if ( url_it != clients.end() ) {
      return url_it->second;
    } else {
      MCCConfig cfg;
      usercfg.ApplyToConfig(cfg);
      EMIESClient* ac = new EMIESClient(url, cfg, usercfg.Timeout());
      return clients[url] = ac;
    }
  }

  bool SubmitterEMIES::releaseClient(const URL& url) {
  }

  bool SubmitterEMIES::deleteAllClients() {
    std::map<URL, EMIESClient*>::iterator it;
    for (it = clients.begin(); it != clients.end(); it++) {
        if ((*it).second != NULL) delete (*it).second;
    }
  }

  bool SubmitterEMIES::Submit(const JobDescription& jobdesc,
                             const ExecutionTarget& et, Job& job) {

    EMIESClient* ac = acquireClient(et.url);

    JobDescription modjobdesc(jobdesc);

    if (!ModifyJobDescription(modjobdesc, et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      releaseClient(et.url);
      return false;
    }

    std::string product;
    if (!modjobdesc.UnParse(product, "nordugrid:jsdl")) { // TODO: EMI ES job desc. lang.
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "nordugrid:jsdl");
      releaseClient(et.url);
      return false;
    }

    EMIESJob jobid;
    EMIESJobState jobstate;
    if (!ac->submit(product, jobid, jobstate, et.url.Protocol() == "https")) {
      releaseClient(et.url);
      return false;
    }

    if (!jobid) {
      logger.msg(INFO, "No valid job identifier returned by EMI ES");
      releaseClient(et.url);
      return false;
    }

    if(!jobid.manager) jobid.manager = et.url;



    // TODO: wait for proper state before starting file uploads
    URL stageurl(jobid.stagein);
    if (!PutFiles(modjobdesc, stageurl)) {
      logger.msg(INFO, "Failed uploading local input files");
      releaseClient(et.url);
      return false;
    }

    // URL-izing job id
    URL jobidu(jobid.manager);
    jobidu.AddOption("emiesjobid",jobid.id,true);

    AddJobDetails(modjobdesc, jobidu, et.Cluster, jobid.manager, job);

    releaseClient(et.url);
    return true;
  }

  bool SubmitterEMIES::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    // TODO: redo after EMI ES job description is implemented

    // Check for identical file names.
    bool executableIsAdded(false), inputIsAdded(false), outputIsAdded(false), errorIsAdded(false), logDirIsAdded(false);
    bool targets_exist(false);
    for (std::list<FileType>::const_iterator it1 = jobdesc.Files.begin();
         it1 != jobdesc.Files.end(); it1++) {
      for (std::list<FileType>::const_iterator it2 = it1;
           it2 != jobdesc.Files.end(); it2++) {
        if (it1 == it2) {
          continue;
        }

        targets_exist = (!it1->Target.empty() && !it2->Target.empty());
        if (it1->Name == it2->Name && ((!it1->Source.empty() && !it2->Source.empty()) ||
                                        targets_exist)) {
          if (targets_exist){
             // When the output file would be sent more than one targets
             if ( !(it1->Target.front() == it2->Target.front()) ) continue;
          }
          logger.msg(ERROR, "Two files have identical file name '%s'.", it1->Name);
          return false;
        }

      }

      executableIsAdded  |= (it1->Name == jobdesc.Application.Executable.Name);
      inputIsAdded       |= (it1->Name == jobdesc.Application.Input);
      outputIsAdded      |= (it1->Name == jobdesc.Application.Output);
      errorIsAdded       |= (it1->Name == jobdesc.Application.Error);
      logDirIsAdded      |= (it1->Name == jobdesc.Application.LogDir);
    }

    if (!executableIsAdded &&
        !Glib::path_is_absolute(jobdesc.Application.Executable.Name)) {
      FileType file;
      file.Name = jobdesc.Application.Executable.Name;
      file.Source.push_back(URL(file.Name));
      file.KeepData = false;
      file.IsExecutable = true;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.Input.empty() && !inputIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Input;
      file.Source.push_back(file.Name);
      file.KeepData = false;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.Output.empty() && !outputIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Output;
      file.KeepData = true;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.Error.empty() && jobdesc.Application.Output != jobdesc.Application.Error && !errorIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Error;
      file.KeepData = true;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.LogDir.empty() && !logDirIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.LogDir;
      file.KeepData = true;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
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

  bool SubmitterEMIES::Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job) {
    logger.msg(VERBOSE, "Migration for EMI ES is not implemented");
    return false;
  }


} // namespace Arc
