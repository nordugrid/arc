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
    return true;
  }

  bool SubmitterEMIES::deleteAllClients() {
    std::map<URL, EMIESClient*>::iterator it;
    for (it = clients.begin(); it != clients.end(); it++) {
        if ((*it).second != NULL) delete (*it).second;
    }
    return true;
  }

  bool SubmitterEMIES::Submit(const JobDescription& jobdesc,
                             const ExecutionTarget& et, Job& job) {
    // TODO: this is multi step process. So having retries would be nice.

    EMIESClient* ac = acquireClient(et.url);

    JobDescription modjobdesc(jobdesc);

    if (!ModifyJobDescription(modjobdesc, et)) {
      logger.msg(INFO, "Failed adapting job description to target resources");
      releaseClient(et.url);
      return false;
    }

    std::string product;
    if (!modjobdesc.UnParse(product, "emies:adl")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "emies:adl");
      releaseClient(et.url);
      return false;
    }

    EMIESJob jobid;
    EMIESJobState jobstate;
    if (!ac->submit(product, jobid, jobstate, et.url.Protocol() == "https")) {
      logger.msg(INFO, "Failed to submit job description");
      releaseClient(et.url);
      return false;
    }

    if (!jobid) {
      logger.msg(INFO, "No valid job identifier returned by EMI ES");
      releaseClient(et.url);
      return false;
    }

    if(!jobid.manager) jobid.manager = et.url;

    for(;;) {
      if(jobstate.HasAttribute("CLIENT-STAGEIN-POSSIBLE")) break;
      if(jobstate.state == "TERMINAL") {
        logger.msg(INFO, "Job failed on service side");
        releaseClient(et.url);
        return false;
      }
      // If service jumped over stageable state client probably does not
      // have to send anything.
      if((jobstate.state != "ACCEPTED") && (jobstate.state != "PREPROCESSING")) break;
      sleep(5);
      if(!ac->stat(jobid, jobstate)) {
        logger.msg(INFO, "Failed to obtain state of job");
        releaseClient(et.url);
        return false;
      };
    }

    if(jobstate.HasAttribute("CLIENT-STAGEIN-POSSIBLE")) {
      URL stageurl(jobid.stagein);
      if (!PutFiles(modjobdesc, stageurl)) {
        logger.msg(INFO, "Failed uploading local input files");
        releaseClient(et.url);
        return false;
      }
      // It is not clear how service is implemented. So notifying should not harm.
      if (!ac->notify(jobid)) {
        logger.msg(INFO, "Failed to notify service");
        releaseClient(et.url);
        return false;
      }
    } else {
      // TODO: check if client has files to send
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
    // Check if executable and input is contained in the file list.
    bool executableIsAdded(false), inputIsAdded(false), outputIsAdded(false), errorIsAdded(false), logDirIsAdded(false);
    for (std::list<InputFileType>::iterator it1 = jobdesc.DataStaging.InputFiles.begin();
         it1 != jobdesc.DataStaging.InputFiles.end(); ++it1) {
      std::list<InputFileType>::const_iterator it2 = it1;
      for (++it2; it2 != jobdesc.DataStaging.InputFiles.end(); ++it2) {
        if (it1->Name == it2->Name && !it1->Sources.empty() && !it2->Sources.empty()) {
          logger.msg(ERROR, "Two files have identical file name '%s'.", it1->Name);
          return false;
        }
      }

      if (!it1->Sources.empty() && it1->Sources.front().Protocol() == "file" && !Glib::file_test(it1->Sources.front().Path(), Glib::FILE_TEST_EXISTS)) {
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

  bool SubmitterEMIES::Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job) {
    logger.msg(VERBOSE, "Migration for EMI ES is not implemented");
    return false;
  }


} // namespace Arc
