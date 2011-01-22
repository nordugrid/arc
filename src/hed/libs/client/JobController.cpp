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

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/client/Broker.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Submitter.h>
#include <arc/client/TargetGenerator.h>
#include <arc/UserConfig.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/FileCache.h>
#include <arc/data/URLMap.h>
#include <arc/loader/FinderLoader.h>
#include "JobController.h"

namespace Arc {

  Logger JobController::logger(Logger::getRootLogger(), "JobController");

  JobController::JobController(const UserConfig& usercfg,
                               const std::string& flavour)
    : flavour(flavour),
      data_source(NULL),
      data_destination(NULL),
      usercfg(usercfg) {}

  JobController::~JobController() {
    if(data_source) delete data_source;
    if(data_destination) delete data_destination;
  }

  void JobController::FillJobStore(const std::list<URL>& jobids) {

    if (!usercfg.JobListFile().empty()) {
      logger.msg(VERBOSE, "Using job list file %s", usercfg.JobListFile());
      FileLock lock(usercfg.JobListFile());
      jobstorage.ReadFromFile(usercfg.JobListFile());
    }
    else {
      logger.msg(ERROR, "Job controller has no job list configuration");
      return;
    }

    if (!jobids.empty()) {
      logger.msg(VERBOSE, "Filling job store for %s with jobs according to "
                 "specified jobIDs", flavour);

      for (std::list<URL>::const_iterator it = jobids.begin();
           it != jobids.end(); it++) {
        XMLNodeList xmljobs;
        xmljobs = jobstorage.XPathLookup("//Job[IDFromEndpoint='" +
                                         it->str() + "']", NS());
        if (xmljobs.empty())
          // Included for backwards compatibility.
          xmljobs = jobstorage.XPathLookup("//Job[JobID='" +
                                           it->str() + "']", NS());
        if (xmljobs.empty()) {
          logger.msg(VERBOSE, "Job not found in the job list: %s", it->str());
          continue;
        }
        if (flavour == (std::string)xmljobs.front()["Flavour"])
          jobstore.push_back(xmljobs.front());
      }
    }

    if (!usercfg.GetSelectedServices(COMPUTING).empty()) {
      logger.msg(VERBOSE, "Filling job store for %s with jobs according to "
                 "the list of selected resources", flavour);

      XMLNodeList xmljobs =
        jobstorage.XPathLookup("//Job[Flavour='" + flavour + "']", NS());

      for (std::list<std::string>::const_iterator it =
             usercfg.GetSelectedServices(COMPUTING).begin();
           it != usercfg.GetSelectedServices(COMPUTING).end(); it++) {
        std::string::size_type pos = it->find(":");
        if (pos != std::string::npos) {
          std::string flav = it->substr(0, pos);
          if (flav == flavour || flav == "*" || flav.empty()) {
            URL url = CreateURL(it->substr(pos + 1), COMPUTING);
            for (XMLNodeList::iterator xit = xmljobs.begin();
                 xit != xmljobs.end(); xit++) {
              URL cluster = (std::string)(*xit)["Cluster"];
              if (cluster == url)
                jobstore.push_back(*xit);
            }
          }
        }
      }
    }

    if (!jobstore.empty() && !usercfg.GetRejectedServices(COMPUTING).empty()) {
      logger.msg(VERBOSE, "Removing jobs from job store for %s according to "
                 "list of rejected resources", flavour);

      for (std::list<std::string>::const_iterator it =
             usercfg.GetRejectedServices(COMPUTING).begin();
           it != usercfg.GetRejectedServices(COMPUTING).end(); it++) {
        std::string::size_type pos = it->find(":");
        if (pos != std::string::npos) {
          std::string flav = it->substr(0, pos);
          if (flav == flavour || flav == "*" || flav.empty()) {
            URL url = CreateURL(it->substr(pos + 1), COMPUTING);
            std::list<Job>::iterator jit = jobstore.begin();
            while (jit != jobstore.end()) {
              if (jit->Cluster == url) {
                logger.msg(VERBOSE, "Removing job %s from job store since it "
                           "runs on a rejected resource", jit->JobID.str());
                jit = jobstore.erase(jit);
              }
              else
                jit++;
            }
          }
        }
      }
    }

    if (jobids.empty() && usercfg.GetSelectedServices(COMPUTING).empty()) {
      logger.msg(VERBOSE, "Filling job store for %s with all jobs, except "
                 "those running on rejected resources", flavour);

      XMLNodeList xmljobs =
        jobstorage.XPathLookup("//Job[Flavour='" + flavour + "']", NS());

      for (XMLNodeList::iterator xit = xmljobs.begin();
           xit != xmljobs.end(); xit++) {
        bool reject = false;
        URL cluster = (std::string)(*xit)["Cluster"];
        for (std::list<std::string>::const_iterator it =
               usercfg.GetRejectedServices(COMPUTING).begin();
             it != usercfg.GetRejectedServices(COMPUTING).end(); it++) {
          std::string::size_type pos = it->find(":");
          if (pos != std::string::npos) {
            std::string flav = it->substr(0, pos);
            if (flav == flavour || flav == "*" || flav.empty())
              if (cluster == CreateURL(it->substr(pos + 1), COMPUTING)) {
                reject = true;
                break;
              }
          }
        }
        if (!reject)
          jobstore.push_back(*xit);
      }
    }

    logger.msg(VERBOSE, "Job store for %s contains %ld jobs",
               flavour, jobstore.size());
  }

  void JobController::FillJobStore(const Job& job) {

    if (job.Flavour != flavour) {
      logger.msg(WARNING, "The middleware flavour of the job (%s) does not match that of the job controller (%s)", job.Flavour, flavour);
      return;
    }

    if (!job.JobID) {
      logger.msg(WARNING, "The job ID (%s) is not a valid URL", job.JobID.str());
      return;
    }

    if (!job.Cluster) {
      logger.msg(WARNING, "The resource URL is not a valid URL", job.Cluster.str());
      return;
    }

    jobstore.push_back(job);
  }

  bool JobController::Get(const std::list<std::string>& status,
                          const std::string& downloaddir,
                          const bool keep,
                          const bool usejobname) {
    std::list<URL> toberemoved;

    GetJobInformation();

    std::list<Job*> downloadable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (!it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      if (it->State == JobState::DELETED) {
        logger.msg(WARNING, "Job has already been deleted: %s",
                   it->JobID.str());
        continue;
      }
      else if (!it->State.IsFinished()) {
        logger.msg(WARNING, "Job has not finished yet: %s", it->JobID.str());
        continue;
      }

      downloadable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = downloadable.begin();
         it != downloadable.end(); it++) {

      bool downloaded = GetJob(**it, downloaddir, usejobname);
      if (!downloaded) {
        logger.msg(ERROR, "Failed downloading job %s", (*it)->JobID.str());
        ok = false;
        continue;
      }

      if (!keep) {
        bool cleaned = CleanJob(**it, true);
        if (!cleaned) {
          logger.msg(ERROR, "Failed cleaning job %s", (*it)->JobID.str());
          ok = false;
          continue;
        }
        toberemoved.push_back((*it)->JobID);
      }
    }

    if (toberemoved.size() > 0)
      RemoveJobs(toberemoved);

    return ok;
  }

  bool JobController::Kill(const std::list<std::string>& status,
                           const bool keep) {
    std::list<URL> toberemoved;

    GetJobInformation();

    std::list<Job*> killable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (!it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      if (it->State == JobState::DELETED) {
        logger.msg(WARNING, "Job has already been deleted: %s", it->JobID.str());
        continue;
      }
      else if (it->State.IsFinished()) {
        logger.msg(WARNING, "Job has already finished: %s", it->JobID.str());
        continue;
      }

      killable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = killable.begin();
         it != killable.end(); it++) {

      bool cancelled = CancelJob(**it);
      if (!cancelled) {
        logger.msg(ERROR, "Failed cancelling job %s", (*it)->JobID.str());
        ok = false;
        continue;
      }

      if (!keep) {
        bool cleaned = CleanJob(**it, true);
        if (!cleaned) {
          logger.msg(ERROR, "Failed cleaning job %s", (*it)->JobID.str());
          ok = false;
          continue;
        }
        toberemoved.push_back((*it)->JobID.str());
      }
    }

    if (toberemoved.size() > 0)
      RemoveJobs(toberemoved);

    return ok;
  }

  bool JobController::Clean(const std::list<std::string>& status,
                            const bool force) {
    std::list<URL> toberemoved;

    GetJobInformation();

    std::list<Job*> cleanable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (!it->State && force && status.empty()) {
        logger.msg(WARNING, "Job information not found, job %s will only be deleted from local joblist",
                   it->JobID.str());
        toberemoved.push_back(it->JobID);
        continue;
      }

      if (!it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      // Job state is not among the specified states.
      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      if (!it->State.IsFinished()) {
        if (force)
          toberemoved.push_back(it->JobID);
        else
          logger.msg(WARNING, "Job has not finished yet: %s", it->JobID.str());
        continue;
      }

      cleanable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = cleanable.begin();
         it != cleanable.end(); it++) {
      bool cleaned = CleanJob(**it, force);
      if (!cleaned) {
        if (force)
          toberemoved.push_back((*it)->JobID);
        logger.msg(ERROR, "Failed cleaning job %s", (*it)->JobID.str());
        ok = false;
        continue;
      }
      toberemoved.push_back((*it)->JobID);
    }

    if (toberemoved.size() > 0)
      RemoveJobs(toberemoved);

    return ok;
  }

  bool JobController::Cat(const std::list<std::string>& status,
                          const std::string& whichfile) {
    logger.msg(WARNING, "The JobController::Cat(const std::list<std::string>&, const std::string&) method is DEPRECATED, use the JobController::Cat(std::ostream&, const std::list<std::string>&, const std::string&) method instead.");
    return Cat(std::cout, status, whichfile);
  }

  bool JobController::Cat(std::ostream& out,
                          const std::list<std::string>& status,
                          const std::string& whichfile_) {
    std::string whichfile(whichfile_);
    if (whichfile == "joblog") {
      logger.msg(WARNING, "Specifying the \"gmlog\" value for the whichfile parameter in the Job::Cat method is DEPRECATED, use the \"joblog\" value instead.");
      whichfile = "joblog";
    }

    if (whichfile != "stdout" && whichfile != "stderr" && whichfile != "joblog") {
      logger.msg(ERROR, "Unknown output %s", whichfile);
      return false;
    }

    GetJobInformation();

    std::list<Job*> catable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (!it->State) {
        logger.msg(WARNING, "Job state information not found: %s",
                   it->JobID.str());
        continue;
      }

      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      if (it->State == JobState::DELETED) {
        logger.msg(WARNING, "Job deleted: %s", it->JobID.str());
        continue;
      }

      // The job-log might be available before the job has started (middleware dependent).
      if (whichfile != "joblog" &&
          !it->State.IsFinished() &&
          it->State != JobState::RUNNING &&
          it->State != JobState::FINISHING) {
        logger.msg(WARNING, "Job has not started yet: %s", it->JobID.str());
        continue;
      }

      if (((whichfile == "stdout") && (it->StdOut.empty())) ||
          ((whichfile == "stderr") && (it->StdErr.empty())) ||
          ((whichfile == "joblog") && (it->LogDir.empty()))) {
        logger.msg(ERROR, "Can not determine the %s location: %s",
                   whichfile, it->JobID.str());
        continue;
      }

      catable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = catable.begin();
         it != catable.end(); it++) {
      std::string filename = Glib::build_filename(Glib::get_tmp_dir(), "arccat.XXXXXX");
      int tmp_h = Glib::mkstemp(filename);
      if (tmp_h == -1) {
        logger.msg(INFO, "Could not create temporary file \"%s\"", filename);
        logger.msg(ERROR, "Cannot create output of %s for job (%s)", whichfile, (*it)->JobID.str());
        ok = false;
        continue;
      }
      close(tmp_h);

      logger.msg(VERBOSE, "Catting %s for job %s", whichfile, (*it)->JobID.str());

      URL src = GetFileUrlForJob((**it), whichfile);
      if (!src) {
        logger.msg(ERROR, "Cannot create output of %s for job (%s): Invalid source %s", whichfile, (*it)->JobID.str(), src.str());
        continue;
      }

      URL dst(filename);
      if (!dst) {
        logger.msg(ERROR, "Cannot create output of %s for job (%s): Invalid destination %s", whichfile, (*it)->JobID.str(), dst.str());
        continue;
      }

      bool copied = ARCCopyFile(src, dst);

      if (copied) {
        out << IString("%s from job %s", whichfile,
                             (*it)->JobID.str()) << std::endl;
        std::ifstream is(filename.c_str());
        char c;
        while (is.get(c)) {
          out.put(c);
        }
        is.close();
        unlink(filename.c_str());
      }
      else
        ok = false;
    }

    return ok;
  }

  bool JobController::PrintJobStatus(const std::list<std::string>& status,
                                     const bool longlist) {
    logger.msg(WARNING, "The JobController::PrintJobStatus method is DEPRECATED, use the Job::SaveJobStatusToStream method instead.");
    return SaveJobStatusToStream(std::cout, status, longlist);
  }

  bool JobController::SaveJobStatusToStream(std::ostream& out,
                                            const std::list<std::string>& status,
                                            bool longlist) {

    GetJobInformation();

    for (std::list<Job>::const_iterator it = jobstore.begin();
         it != jobstore.end(); it++) {
      if (!it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        if (Time() - it->LocalSubmissionTime < 90)
          logger.msg(WARNING, "This job was very recently "
                     "submitted and might not yet "
                     "have reached the information system");
        continue;
      }

      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      it->SaveToStream(out, longlist);
    }
    return true;
  }

  void JobController::FetchJobs(const std::list<std::string>& status,
                                std::vector<const Job*>& jobs) {
    GetJobInformation();

    for (std::list<Job>::const_iterator it = jobstore.begin();
         it != jobstore.end(); it++) {
      if (!it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        if (Time() - it->LocalSubmissionTime < 90)
          logger.msg(WARNING, "This job was very recently "
                     "submitted and might not yet "
                     "have reached the information system");
        continue;
      }

      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      jobs.push_back(&*it);
    }
  }

  bool JobController::Migrate(TargetGenerator& targetGen,
                              Broker *broker,
                              const UserConfig& usercfg,
                              const bool forcemigration,
                              std::list<URL>& migratedJobIDs) {
    bool retVal = true;
    std::list<URL> toberemoved;

    GetJobInformation();

    std::list<Job> migratedJobs;
    for (std::list<Job>::iterator itJob = jobstore.begin(); itJob != jobstore.end(); itJob++) {
      if (itJob->State != JobState::QUEUING) {
        logger.msg(WARNING, "Cannot migrate job %s, it is not queuing.", itJob->JobID.str());
        continue;
      }

      JobDescription jobDesc;
      if (!GetJobDescription(*itJob, itJob->JobDescription))
        continue;

      jobDesc.Parse(itJob->JobDescription);

      migratedJobs.push_back(Job());

      broker->PreFilterTargets(targetGen.FoundTargets(), jobDesc);
      while (true) {
        const ExecutionTarget *currentTarget = broker->GetBestTarget();
        if (!currentTarget) {
          logger.msg(ERROR, "Job migration failed, for job %s, no more possible targets", itJob->JobID.str());
          retVal = false;
          migratedJobs.pop_back();
          break;
        }

        if (!currentTarget->Migrate(usercfg, itJob->JobID, jobDesc, forcemigration, migratedJobs.back())) {
          continue;
        }

        broker->RegisterJobsubmission();
        migratedJobIDs.push_back(migratedJobs.back().JobID);
        toberemoved.push_back(URL(itJob->JobID.str()));
        break;
      }
    }

    if (toberemoved.size() > 0) {
      RemoveJobs(toberemoved);
    }

    {
      FileLock lock(usercfg.JobListFile());
      Config jobstorage;
      jobstorage.ReadFromFile(usercfg.JobListFile());
      for (std::list<Job>::const_iterator it = migratedJobs.begin();
           it != migratedJobs.end(); it++) {
        XMLNode xJob = jobstorage.NewChild("Job");
        it->ToXML(xJob);
      }
      jobstorage.SaveToFile(usercfg.JobListFile());
    }

    return retVal;
  }

  bool JobController::Renew(const std::list<std::string>& status) {

    GetJobInformation();

    std::list<Job*> renewable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (!it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      if (!it->State.IsFinished()) {
        logger.msg(WARNING, "Job has not finished yet: %s", it->JobID.str());
        continue;
      }

      renewable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = renewable.begin();
         it != renewable.end(); it++) {
      bool renewed = RenewJob(**it);
      if (!renewed) {
        logger.msg(ERROR, "Failed renewing job %s", (*it)->JobID.str());
        ok = false;
        continue;
      }
    }
    return ok;
  }

  bool JobController::Resume(const std::list<std::string>& status) {

    GetJobInformation();

    std::list<Job*> resumable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (!it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() &&
          std::find(status.begin(), status.end(), it->State()) == status.end() &&
          std::find(status.begin(), status.end(), it->State.GetGeneralState()) == status.end())
        continue;

      if (!it->State.IsFinished()) {
        logger.msg(WARNING, "Job has not finished yet: %s", it->JobID.str());
        continue;
      }

      resumable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = resumable.begin();
         it != resumable.end(); it++) {
      bool resumed = ResumeJob(**it);
      if (!resumed) {
        logger.msg(ERROR, "Failed resuming job %s", (*it)->JobID.str());
        ok = false;
        continue;
      }
    }
    return ok;
  }

  std::list<std::string> JobController::GetDownloadFiles(const URL& dir) {

    std::list<std::string> files;
    std::list<FileInfo> outputfiles;

    DataHandle handle(dir, usercfg);
    if (!handle) {
      logger.msg(INFO, "Unable to list files at %s", dir.str());
      return files;
    }
    if(!handle->List(outputfiles, (Arc::DataPoint::DataPointInfoType)
                                  (DataPoint::INFO_TYPE_NAME | DataPoint::INFO_TYPE_TYPE))) {
      logger.msg(INFO, "Unable to list files at %s", dir.str());
      return files;
    }

    for (std::list<FileInfo>::iterator i = outputfiles.begin();
         i != outputfiles.end(); i++) {

      if (i->GetName() == ".." || i->GetName() == ".")
        continue;

      if (i->GetType() == FileInfo::file_type_unknown ||
          i->GetType() == FileInfo::file_type_file)
        files.push_back(i->GetName());
      else if (i->GetType() == FileInfo::file_type_dir) {

        std::string path = dir.Path();
        if (path[path.size() - 1] != '/')
          path += "/";
        URL tmpdir(dir);
        tmpdir.ChangePath(path + i->GetName());
        std::list<std::string> morefiles = GetDownloadFiles(tmpdir);
        std::string dirname = i->GetName();
        if (dirname[dirname.size() - 1] != '/')
          dirname += "/";
        for (std::list<std::string>::iterator it = morefiles.begin();
             it != morefiles.end(); it++)
          files.push_back(dirname + *it);
      }
    }
    return files;
  }

  bool JobController::ARCCopyFile(const URL& src, const URL& dst) {

    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    logger.msg(VERBOSE, "Now copying (from -> to)");
    logger.msg(VERBOSE, " %s -> %s", src.str(), dst.str());

    if ((!data_source) || (!*data_source) ||
        (!(*data_source)->SetURL(src))) {
      if(data_source) delete data_source;
      data_source = new DataHandle(src, usercfg);
    }
    DataHandle& source = *data_source;
    if (!source) {
      logger.msg(ERROR, "Unable to initialise connection to source: %s", src.str());
      return false;
    }

    if ((!data_destination) || (!*data_destination) ||
        (!(*data_destination)->SetURL(dst))) {
      if(data_destination) delete data_destination;
      data_destination = new DataHandle(dst, usercfg);
    }
    DataHandle& destination = *data_destination;
    if (!destination) {
      logger.msg(ERROR, "Unable to initialise connection to destination: %s",
                 dst.str());
      return false;
    }

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
      return false;
    }

    return true;

  }

  bool JobController::RemoveJobs(const std::list<URL>& jobids) {

    logger.msg(VERBOSE, "Removing jobs from job list and job store");

    FileLock lock(usercfg.JobListFile());
    jobstorage.ReadFromFile(usercfg.JobListFile());

    for (std::list<URL>::const_iterator it = jobids.begin();
         it != jobids.end(); it++) {

      XMLNodeList xmljobs;
      xmljobs = jobstorage.XPathLookup("//Job[IDFromEndpoint='" + it->str() + "']", NS());
      if (xmljobs.empty()) { // Included for backwards compatibility.
        xmljobs = jobstorage.XPathLookup("//Job[JobID='" + it->str() + "']", NS());
      }

      if (xmljobs.empty())
        logger.msg(ERROR, "Job %s not found in job list.", it->str());
      else {
        XMLNode& xmljob = *xmljobs.begin();
        if (xmljob) {
          logger.msg(INFO, "Removing job %s from job list file", it->str());
          xmljob.Destroy();
        }
      }

      std::list<Job>::iterator it2 = jobstore.begin();
      while (it2 != jobstore.end()) {
        if (it2->JobID == *it) {
          it2 = jobstore.erase(it2);
          break;
        }
        it2++;
      }
    }

    jobstorage.SaveToFile(usercfg.JobListFile());

    logger.msg(VERBOSE, "Job store for %s now contains %d jobs", flavour, jobstore.size());
    logger.msg(VERBOSE, "Finished removing jobs from job list and job store");

    return true;
  }

  std::list<Job> JobController::GetJobDescriptions(const std::list<std::string>& status,
                                                   const bool getlocal) {

    GetJobInformation();

    // Only selected jobs with specified status
    std::list<Job> gettable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {
      if (!status.empty() && !it->State) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() && std::find(status.begin(), status.end(),
                                       it->State()) == status.end())
        continue;
      gettable.push_back(*it);
    }

    //First try to get descriptions from local job file
    if (getlocal) {
      logger.msg(VERBOSE, "Getting job descriptions from local job file");
      CheckLocalDescription(gettable);
    }
    else
      logger.msg(VERBOSE, "Disregarding job descriptions from local job file");

    // Try to get description from cluster
    for (std::list<Job>::iterator it = gettable.begin();
         it != gettable.end();) {
      if (!it->JobDescription.empty()) {
        it++;
        continue;
      }
      if (GetJobDescription(*it, it->JobDescription)) {
        logger.msg(VERBOSE, "Got job description for %s", it->JobID.str());
        it++;
      }
      else {
        logger.msg(INFO, "Failed getting job description for %s", it->JobID.str());
        it = gettable.erase(it);
      }
    }
    return gettable;

  }

  void JobController::CheckLocalDescription(std::list<Job>& jobs) {
    for (std::list<Job>::iterator it = jobs.begin();
         it != jobs.end();) {
      // Search for jobids
      XMLNodeList xmljobs;
      xmljobs = jobstorage.XPathLookup("//Job[IDFromEndpoint='" + it->JobID.str() + "']", NS());
      if (xmljobs.empty()) { // Included for backwards compatibility.
        xmljobs = jobstorage.XPathLookup("//Job[JobID='" + it->JobID.str() + "']", NS());
      }

      if (xmljobs.empty()) {
        logger.msg(INFO, "Job not found in job list: %s", it->JobID.str());
        it++;
        continue;
      }
      XMLNode& xmljob = *xmljobs.begin();

      if (xmljob["JobDescription"]) {
        JobDescription jobdesc;

        // Check for valid job description
        if (jobdesc.Parse((std::string)xmljob["JobDescription"]))
          logger.msg(VERBOSE, "Valid job description found for: %s", it->JobID.str());
        else {
          logger.msg(INFO, "Invalid job description found for: %s", it->JobID.str());
          it++;
          continue;
        }

        // Check checksums of local input files
        bool CKSUM = true;
        int size = xmljob["LocalInputFiles"].Size();
        for (int i = 0; i < size; i++) {
          const std::string file = (std::string)xmljob["LocalInputFiles"]["File"][i]["Source"];
          const std::string cksum_old = (std::string)xmljob["LocalInputFiles"]["File"][i]["CheckSum"];
          const std::string cksum_new = Submitter::GetCksum(file, usercfg);
          if (cksum_old != cksum_new) {
            logger.msg(WARNING, "Checksum of input file %s has changed.", file);
            CKSUM = false;
          }
          else
            logger.msg(VERBOSE, "Stored and new checksum of input file %s are identical.", file);
        }
        // Push_back job and job descriptions
        if (CKSUM) {
          logger.msg(INFO, "Job description for %s retrieved locally", it->JobID.str());
          it->JobDescription = (std::string)xmljob["JobDescription"];
          it++;
        }
        else {
          logger.msg(WARNING, "Job %s can not be resubmitted", it->JobID.str());
          it = jobs.erase(it);
        }
      }
      else {
        logger.msg(INFO, "Job description for %s could not be retrieved locally", it->JobID.str());
        it++;
      }
    } //end loop over jobs
    return;
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
      logger.msg(ERROR, "JobController plugin \"%s\" not found.", name);
      return NULL;
    }

    JobControllerPluginArgument arg(usercfg);
    JobController *jobcontroller =
      factory_->GetInstance<JobController>("HED:JobController", name, &arg, false);

    if (!jobcontroller) {
      logger.msg(ERROR, "JobController %s could not be created", name);
      return NULL;
    }

    jobcontrollers.push_back(jobcontroller);
    logger.msg(INFO, "Loaded JobController %s", name);
    return jobcontroller;
  }

} // namespace Arc
