// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <fstream>
#include <iostream>

#include <unistd.h>
#include <glibmm/fileutils.h>
#include <glibmm.h>

#include <arc/IString.h>
#include <arc/UserConfig.h>
#include <arc/data/DataMover.h>
#include <arc/data/FileCache.h>
#include <arc/data/URLMap.h>
#include <arc/loader/FinderLoader.h>

#include "JobController.h"

namespace Arc {

  Logger JobController::logger(Logger::getRootLogger(), "JobController");

  JobController::JobController(const UserConfig& usercfg,
                               const std::string& flavour)
    : flavour(flavour),
      usercfg(usercfg),
      data_source(NULL),
      data_destination(NULL) {}

  JobController::~JobController() {
    if(data_source) delete data_source;
    if(data_destination) delete data_destination;
  }

  bool JobController::FillJobStore(const Job& job) {

    if (job.Flavour != flavour) {
      logger.msg(WARNING, "The middleware flavour of the job (%s) does not match that of the job controller (%s)", job.Flavour, flavour);
      return false;
    }

    if (!job.JobID) {
      logger.msg(WARNING, "The job ID (%s) is not a valid URL", job.JobID.fullstr());
      return false;
    }

    if (!job.Cluster) {
      logger.msg(WARNING, "The resource URL is not a valid URL", job.Cluster.str());
      return false;
    }

    jobstore.push_back(job);
    return true;
  }

  bool JobController::ListFilesRecursive(const URL& dir, std::list<std::string>& files, const std::string& prefix) const {
    std::list<FileInfo> outputfiles;

    DataHandle handle(dir, usercfg);
    if (!handle) {
      logger.msg(INFO, "Unable to list files at %s", dir.str());
      return false;
    }
    if(!handle->List(outputfiles, (Arc::DataPoint::DataPointInfoType)
                                  (DataPoint::INFO_TYPE_NAME | DataPoint::INFO_TYPE_TYPE))) {
      logger.msg(INFO, "Unable to list files at %s", dir.str());
      return false;
    }

    for (std::list<FileInfo>::iterator i = outputfiles.begin();
         i != outputfiles.end(); i++) {
      if (i->GetName() == ".." || i->GetName() == ".") {
        continue;
      }

      if (i->GetType() == FileInfo::file_type_unknown ||
          i->GetType() == FileInfo::file_type_file) {
        files.push_back(prefix + i->GetName());
      }
      else if (i->GetType() == FileInfo::file_type_dir) {
        std::string path = dir.Path();
        if (path[path.size() - 1] != '/') {
          path += "/";
        }
        URL tmpdir(dir);
        tmpdir.ChangePath(path + i->GetName());

        std::string dirname = i->GetName();
        if (dirname[dirname.size() - 1] != '/') {
          dirname += "/";
        }
        if (!ListFilesRecursive(tmpdir, files, dirname)) {
          return false;
        }
      }
    }

    return true;
  }

  bool JobController::CopyJobFile(const URL& src, const URL& dst) const {
    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    logger.msg(VERBOSE, "Now copying (from -> to)");
    logger.msg(VERBOSE, " %s -> %s", src.str(), dst.str());

    URL src_(src);
    URL dst_(dst);
    src_.AddOption("checksum=no");
    dst_.AddOption("checksum=no");

    if ((!data_source) || (!*data_source) ||
        (!(*data_source)->SetURL(src_))) {
      if(data_source) delete data_source;
      data_source = new DataHandle(src_, usercfg);
    }
    DataHandle& source = *data_source;
    if (!source) {
      logger.msg(ERROR, "Unable to initialise connection to source: %s", src.str());
      return false;
    }

    if ((!data_destination) || (!*data_destination) ||
        (!(*data_destination)->SetURL(dst_))) {
      if(data_destination) delete data_destination;
      data_destination = new DataHandle(dst_, usercfg);
    }
    DataHandle& destination = *data_destination;
    if (!destination) {
      logger.msg(ERROR, "Unable to initialise connection to destination: %s",
                 dst.str());
      return false;
    }

    // Set desired number of retries. Also resets any lost
    // tries from previous files.
    source->SetTries((src.Protocol() == "file")?1:3);
    destination->SetTries((dst.Protocol() == "file")?1:3);

    // Turn off all features we do not need
    source->SetAdditionalChecks(false);
    destination->SetAdditionalChecks(false);

    FileCache cache;
    DataStatus res =
      mover.Transfer(*source, *destination, cache, URLMap(), 0, 0, 0,
                     usercfg.Timeout());
    if (!res.Passed()) {
      if (!res.GetDesc().empty())
        logger.msg(ERROR, "File download failed: %s - %s", std::string(res), res.GetDesc());
      else
        logger.msg(ERROR, "File download failed: %s", std::string(res));
      // Reset connection because one can't be sure how failure
      // affects server and/or connection state.
      // TODO: Investigate/define DMC behavior in such case.
      delete data_source;
      data_source = NULL;
      delete data_destination;
      data_destination = NULL;
      return false;
    }

    return true;
  }

  std::list<Job> JobController::GetJobDescriptions(const std::list<std::string>& status,
                                                   bool getlocal) {
    GetJobInformation();

    // Only selected jobs with specified status
    std::list<Job> gettable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {
      if (!status.empty() && !it->State) {
        continue;
      }

      if (!status.empty() && std::find(status.begin(), status.end(),
                                       it->State()) == status.end()) {
        continue;
      }

      gettable.push_back(*it);
    }

    // Get job description for those jobs without one.
    for (std::list<Job>::iterator it = gettable.begin();
         it != gettable.end();) {
      if (!it->JobDescriptionDocument.empty()) {
        it++;
        continue;
      }
      if (GetJobDescription(*it, it->JobDescriptionDocument)) {
        logger.msg(VERBOSE, "Job description retrieved from execution service for job (%s)", it->IDFromEndpoint.fullstr());
        it++;
      }
      else {
        logger.msg(INFO, "Failed retrieving job description for job (%s)", it->IDFromEndpoint.fullstr());
        it = gettable.erase(it);
      }
    }

    return gettable;
  }

  JobControllerLoader::JobControllerLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  JobControllerLoader::~JobControllerLoader() {
    for (std::list<JobController*>::iterator it = jobcontrollers.begin();
         it != jobcontrollers.end(); it++)
      delete *it;
  }

  JobController* JobControllerLoader::load(const std::string& name,
                                           const UserConfig& usercfg) {
    if (name.empty())
      return NULL;

    if(!factory_->load(FinderLoader::GetLibrariesList(),
                       "HED:JobController", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "JobController plugin \"%s\" not found.", name);
      return NULL;
    }

    JobControllerPluginArgument arg(usercfg);
    JobController *jobcontroller =
      factory_->GetInstance<JobController>("HED:JobController", name, &arg, false);

    if (!jobcontroller) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "JobController %s could not be created", name);
      return NULL;
    }

    jobcontrollers.push_back(jobcontroller);
    logger.msg(DEBUG, "Loaded JobController %s", name);
    return jobcontroller;
  }

} // namespace Arc
