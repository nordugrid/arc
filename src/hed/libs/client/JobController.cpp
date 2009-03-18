// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <fstream>
#include <iostream>

#include <unistd.h>
#include <glibmm/fileutils.h>

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Submitter.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/FileCache.h>
#include <arc/data/URLMap.h>
#include "Sandbox.h"
#include "JobController.h"

namespace Arc {

  Logger JobController::logger(Logger::getRootLogger(), "JobController");

  JobController::JobController(Config *cfg, const std::string& flavour)
    : ACC(cfg, flavour),
      joblist((*cfg)["JobList"]),
      usercfg((*cfg)["UserConfig"]) {}

  JobController::~JobController() {}

  void JobController::FillJobStore(const std::list<URL>& jobids,
                                   const std::list<URL>& clusterselect,
                                   const std::list<URL>& clusterreject) {

    if (!joblist.empty()) {
      logger.msg(DEBUG, "Using job list file %s", joblist);
      FileLock lock(joblist);
      jobstorage.ReadFromFile(joblist);
    }
    else {
      logger.msg(ERROR, "Job controller has no job list configuration");
      return;
    }

    if (!jobids.empty()) {
      logger.msg(DEBUG, "Filling job store with jobs according to "
                 "specified jobids");

      for (std::list<URL>::const_iterator it = jobids.begin();
           it != jobids.end(); it++) {

        XMLNodeList xmljobs =
          jobstorage.XPathLookup("//Job[JobID='" + it->str() + "']", NS());

        if (xmljobs.empty()) {
          logger.msg(DEBUG, "Job not found in job list: %s", it->str());
          continue;
        }

        XMLNode& xmljob = *xmljobs.begin();

        if (flavour == (std::string)xmljob["Flavour"]) {
          Job job;
          job.JobID = (std::string)xmljob["JobID"];
          job.Flavour = (std::string)xmljob["Flavour"];
          job.Cluster = (std::string)xmljob["Cluster"];
          job.SubmissionEndpoint = (std::string)xmljob["SubmissionEndpoint"];
          job.InfoEndpoint = (std::string)xmljob["InfoEndpoint"];
          job.ISB = (std::string)xmljob["ISB"];
          job.OSB = (std::string)xmljob["OSB"];
          job.StdOut = (std::string)xmljob["StdOut"];
          job.StdErr = (std::string)xmljob["StdErr"];
          job.AuxURL = (std::string)xmljob["AuxURL"];
          job.AuxInfo = (std::string)xmljob["AuxInfo"];
          job.LocalSubmissionTime = (std::string)xmljob["LocalSubmissionTime"];
          for (int i = 0; (bool)xmljob["OldJobID"][i]; i++)
            job.OldJobIDs.push_back(URL((std::string)xmljob["OldJobID"][i]));
          jobstore.push_back(job);
        }
      }
    }

    if (!clusterselect.empty()) {
      logger.msg(DEBUG, "Filling job store with jobs according to list of "
                 "selected clusters");

      XMLNodeList xmljobs =
        jobstorage.XPathLookup("//Job[Flavour='" + flavour + "']", NS());

      for (XMLNodeList::iterator it = xmljobs.begin();
           it != xmljobs.end(); it++) {

        URL cluster = (std::string)(*it)["Cluster"];

        if (std::find(clusterselect.begin(), clusterselect.end(),
                      cluster) != clusterselect.end()) {
          Job job;
          job.JobID = (std::string)(*it)["JobID"];
          job.Flavour = (std::string)(*it)["Flavour"];
          job.Cluster = (std::string)(*it)["Cluster"];
          job.SubmissionEndpoint = (std::string)(*it)["SubmissionEndpoint"];
          job.InfoEndpoint = (std::string)(*it)["InfoEndpoint"];
          job.ISB = (std::string)(*it)["ISB"];
          job.OSB = (std::string)(*it)["OSB"];
          job.StdOut = (std::string)(*it)["StdOut"];
          job.StdErr = (std::string)(*it)["StdErr"];
          job.AuxURL = (std::string)(*it)["AuxURL"];
          job.AuxInfo = (std::string)(*it)["AuxInfo"];
          job.LocalSubmissionTime = (std::string)(*it)["LocalSubmissionTime"];
          for (int i = 0; (bool)(*it)["OldJobID"][i]; i++)
            job.OldJobIDs.push_back(URL((std::string)(*it)["OldJobID"][i]));
          jobstore.push_back(job);
        }
      }
    }

    if (!clusterreject.empty())
      if (!jobstore.empty()) {
        logger.msg(DEBUG, "Removing jobs from job store according to list of "
                   "rejected clusters");

        std::list<Job>::iterator it = jobstore.begin();
        while (it != jobstore.end())
          if (std::find(clusterreject.begin(), clusterreject.end(),
                        it->Cluster) != clusterreject.end()) {
            logger.msg(DEBUG, "Removing job %s from job store since it runs "
                       "on a rejected cluster", it->JobID.str());
            it = jobstore.erase(it);
          }
          else
            it++;
      }

    if (jobids.empty() && clusterselect.empty()) {
      logger.msg(DEBUG, "Filling job store with all jobs, except those "
                 "running on rejected clusters");

      XMLNodeList xmljobs =
        jobstorage.XPathLookup("//Job[Flavour='" + flavour + "']", NS());

      for (XMLNodeList::iterator it = xmljobs.begin();
           it != xmljobs.end(); it++) {

        URL cluster = (std::string)(*it)["Cluster"];

        if (std::find(clusterreject.begin(), clusterreject.end(),
                      cluster) == clusterreject.end()) {
          Job job;
          job.JobID = (std::string)(*it)["JobID"];
          job.Flavour = (std::string)(*it)["Flavour"];
          job.Cluster = (std::string)(*it)["Cluster"];
          job.SubmissionEndpoint = (std::string)(*it)["SubmissionEndpoint"];
          job.InfoEndpoint = (std::string)(*it)["InfoEndpoint"];
          job.ISB = (std::string)(*it)["ISB"];
          job.OSB = (std::string)(*it)["OSB"];
          job.StdOut = (std::string)(*it)["StdOut"];
          job.StdErr = (std::string)(*it)["StdErr"];
          job.AuxURL = (std::string)(*it)["AuxURL"];
          job.AuxInfo = (std::string)(*it)["AuxInfo"];
          job.LocalSubmissionTime = (std::string)(*it)["LocalSubmissionTime"];
          for (int i = 0; (bool)(*it)["OldJobID"][i]; i++)
            job.OldJobIDs.push_back(URL((std::string)(*it)["OldJobID"][i]));
          jobstore.push_back(job);
        }
      }
    }

    logger.msg(DEBUG, "FillJobStore has finished successfully");
    logger.msg(DEBUG, "Job store for %s contains %ld jobs",
               flavour, jobstore.size());
  }

  bool JobController::Get(const std::list<std::string>& status,
                          const std::string& downloaddir,
                          const bool keep,
                          const int timeout) {

    logger.msg(DEBUG, "Getting %s jobs", flavour);
    std::list<URL> toberemoved;

    GetJobInformation();

    std::list<Job*> downloadable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (it->State.empty()) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() && std::find(status.begin(), status.end(),
                                       it->State) == status.end())
        continue;

      /* Need to fix this after we have implemented "normalized states"
         if (it->State == "DELETED") {
         logger.msg(WARNING, "Job has already been deleted: %s",
          it->JobID.str());
         continue;
         }

         if (it->State != "FINISHED" && it->State != "FAILED" &&
          it->State != "KILLED"){
         logger.msg(WARNING, "Job has not finished yet: %s", it->JobID.str());
         continue;
         }
       */

      downloadable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = downloadable.begin();
         it != downloadable.end(); it++) {

      bool downloaded = GetJob(**it, downloaddir);
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

    RemoveJobs(toberemoved);

    return ok;
  }

  bool JobController::Kill(const std::list<std::string>& status,
                           const bool keep,
                           const int timeout) {

    logger.msg(DEBUG, "Killing %s jobs", flavour);
    std::list<URL> toberemoved;

    GetJobInformation();

    std::list<Job*> killable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (it->State.empty()) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() && std::find(status.begin(), status.end(),
                                       it->State) == status.end())
        continue;

      /* Need to fix this after we have implemented "normalized states"
         if (it->State == "DELETED") {
         logger.msg(WARNING, "Job has already been deleted: %s",
          it->JobID.str());
         continue;
         }

         if (it->State == "FINISHED" || it->State == "FAILED" ||
          it->State == "KILLED") {
         logger.msg(WARNING, "Job has already finished: %s", it->JobID.str());
         continue;
         }
       */

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

    RemoveJobs(toberemoved);

    return ok;
  }

  bool JobController::Clean(const std::list<std::string>& status,
                            const bool force,
                            const int timeout) {

    logger.msg(DEBUG, "Cleaning %s jobs", flavour);
    std::list<URL> toberemoved;

    GetJobInformation();

    std::list<Job*> cleanable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (force && it->State.empty() && status.empty()) {
        logger.msg(WARNING, "Job %s will only be deleted from local job list",
                   it->JobID.str());
        toberemoved.push_back(it->JobID);
        continue;
      }

      if (it->State.empty()) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() && std::find(status.begin(), status.end(),
                                       it->State) == status.end())
        continue;

      /* Need to fix this after we have implemented "normalized states"
         if (it->State != "FINISHED" && it->State != "FAILED" &&
          it->State != "KILLED" && it->State != "DELETED") {
         logger.msg(WARNING, "Job has not finished yet: %s", it->JobID.str());
         continue;
         }
       */

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

    RemoveJobs(toberemoved);

    return ok;
  }

  bool JobController::Cat(const std::list<std::string>& status,
                          const std::string& whichfile,
                          const int timeout) {

    logger.msg(DEBUG, "Performing the 'cat' command on %s jobs", flavour);

    GetJobInformation();

    std::list<Job*> catable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {

      if (it->State.empty()) {
        logger.msg(WARNING, "Job state information not found: %s",
                   it->JobID.str());
        continue;
      }

      /* Need to fix this after we have implemented "normalized states"
         if (whichfile == "stdout" || whichfile == "stderr") {
         if(it->State == "DELETED") {
          logger.msg(WARNING, "Job has already been deleted: %s",
            it->JobID.str());
          continue;
         }
         if(it->State == "ACCEPTING" || it->State == "ACCEPTED" ||
           it->State == "PREPARING" || it->State == "PREPARED" ||
           it->State == "INLRMS:Q") {
          logger.msg(WARNING, "Job has not started yet: %s", it->JobID.str());
          continue;
         }
         }
       */

      if (whichfile == "stdout" && it->StdOut.empty()) {
        logger.msg(ERROR, "Can not determine the stdout location: %s",
                   it->JobID.str());
        continue;
      }
      if (whichfile == "stderr" && it->StdErr.empty()) {
        logger.msg(ERROR, "Can not determine the stderr location: %s",
                   it->JobID.str());
        continue;
      }

      catable.push_back(&(*it));
    }

    bool ok = true;
    for (std::list<Job*>::iterator it = catable.begin();
         it != catable.end(); it++) {
      std::string filename("/tmp/arccat.XXXXXX");
      int tmp_h = Glib::mkstemp(filename);
      if (tmp_h == -1) {
        logger.msg(ERROR, "Could not create temporary file \"%s\"", filename);
        ok = false;
        continue;
      }
      close(tmp_h);

      URL src = GetFileUrlForJob((**it), whichfile);
      URL dst(filename);
      bool copied = CopyFile(src, dst);

      if (copied) {
        std::cout << IString("%s from job %s", whichfile,
                             (*it)->JobID.str()) << std::endl;
        std::ifstream is(filename.c_str());
        char c;
        while (is.get(c))
          std::cout.put(c);
        is.close();
        unlink(filename.c_str());
      }
      else
        ok = false;
    }

    return ok;
  }

  bool JobController::Stat(const std::list<std::string>& status,
                           const bool longlist,
                           const int timeout) {

    GetJobInformation();

    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {
      if (it->State.empty()) {
        logger.msg(WARNING, "Job state information not found: %s",
                   it->JobID.str());
        Time now;
        if (now - it->LocalSubmissionTime < 90)
          logger.msg(WARNING, "This job was very recently "
                     "submitted and might not yet "
                     "have reached the information-system");
        continue;
      }
      it->Print(longlist);
    }
    return true;
  }

  bool JobController::Migrate(TargetGenerator& targetGen,
                              Broker* broker,
                              const bool forcemigration,
                              const int timeout) {
    bool retVal = true;

    std::list<URL> migratedJobIDs;

    GetJobInformation();
    // Loop over job descriptions.
    for (std::list<Job>::iterator itJob = jobstore.begin(); itJob != jobstore.end(); itJob++) {
      if (itJob->State != "Running/Executing/Queuing") {
        logger.msg(Arc::WARNING, "Cannot migrate job %s, it is not queuing in the batch system.", itJob->JobID.str());
        continue;
      }

      JobDescription jobDesc;
      if (!GetJobDescription(*itJob, itJob->JobDescription))
        continue;
      if (!PatchInputFileLocation(*itJob, jobDesc))
        continue;

      XMLNode info(NS(), "Job");

      bool endOfList = false;
      broker->PreFilterTargets(targetGen, jobDesc);
      // Try to submit modified JSDL. Only to ARC1 clusters.
      while (true) {
        ExecutionTarget& currentTarget = broker->GetBestTarget(endOfList);
        if (endOfList) {
          logger.msg(Arc::ERROR, "Job migration failed, for job %s, no more possible targets", itJob->JobID.str());
          retVal = false;
          break;
        }

        if (currentTarget.GridFlavour != "ARC1") {
          logger.msg(Arc::WARNING, "Cannot migrate to a %s cluster.", currentTarget.GridFlavour);
          logger.msg(Arc::INFO, "Note: Migration is currently only supported between ARC1 clusters.");
          continue;
        }

        if (!currentTarget.GetSubmitter(usercfg)->Migrate(itJob->JobID, jobDesc, forcemigration, info)) {
          logger.msg(Arc::WARNING, "Migration to %s failed, trying next target", currentTarget.url.str());
          continue;
        }

        // Need to get the JobInnerRepresentation in order to get the number of slots
        JobInnerRepresentation jir;
        jobDesc.getInnerRepresentation(jir);
        // Change number of slots or waiting jobs
        for (std::list<ExecutionTarget>::iterator itTarget = targetGen.ModifyFoundTargets().begin();
             itTarget != targetGen.ModifyFoundTargets().end();
             itTarget++)
          if (currentTarget.url == itTarget->url) {
            if (itTarget->FreeSlots >= abs(jir.Slots)) { // The job will start directly
              itTarget->FreeSlots -= abs(jir.Slots);
              if (itTarget->UsedSlots != -1)
                itTarget->UsedSlots += abs(jir.Slots);
            }
            else   //The job will be queued
              if (itTarget->WaitingJobs != -1)
                itTarget->WaitingJobs += abs(jir.Slots);

          }

        XMLNode xmlDesc;
        jobDesc.getXML(xmlDesc);
        if (xmlDesc["JobDescription"]["JobIdentification"]["JobName"])
          info.NewChild("Name") = (std::string)xmlDesc["JobDescription"]["JobIdentification"]["JobName"];

        info.NewChild("Flavour") = currentTarget.GridFlavour;
        info.NewChild("Cluster") = currentTarget.Cluster.str();
        info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();

        jobstorage.NewChild("Job").Replace(info);

        migratedJobIDs.push_back(URL(itJob->JobID.str()));

        std::cout << Arc::IString("Job migrated with jobid: %s", (std::string)info["JobID"]) << std::endl;
        break;
      } // Loop over all possible targets
    } // Loop over jobs

    if (migratedJobIDs.size() > 0) {
      FileLock lock(joblist);
      jobstorage.SaveToFile(joblist);
    }
    RemoveJobs(migratedJobIDs);   // Saves file aswell.

    return retVal;
  }

  std::list<std::string> JobController::GetDownloadFiles(const URL& dir) {

    std::list<std::string> files;
    std::list<FileInfo> outputfiles;

    DataHandle handle(dir);
    handle->AssignCredentials(proxyPath, certificatePath,
                              keyPath, caCertificatesDir);
    handle->ListFiles(outputfiles, true, false);

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

  bool JobController::CopyFile(const URL& src, const URL& dst) {

    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    logger.msg(DEBUG, "Now copying (from -> to)");
    logger.msg(DEBUG, " %s -> %s", src.str(), dst.str());

    DataHandle source(src);
    if (!source) {
      logger.msg(ERROR, "Failed to get DataHandle on source: %s", src.str());
      return false;
    }
    source->AssignCredentials(proxyPath, certificatePath,
                              keyPath, caCertificatesDir);

    DataHandle destination(dst);
    if (!destination) {
      logger.msg(ERROR, "Failed to get DataHandle on destination: %s",
                 dst.str());
      return false;
    }
    destination->AssignCredentials(proxyPath, certificatePath,
                                   keyPath, caCertificatesDir);

    FileCache cache;
    std::string failure;
    int timeout = 10;
    if (!mover.Transfer(*source, *destination, cache, URLMap(),
                        0, 0, 0, timeout, failure)) {
      if (!failure.empty())
        logger.msg(ERROR, "File download failed: %s", failure);
      else
        logger.msg(ERROR, "File download failed");
      return false;
    }

    return true;

  }

  bool JobController::RemoveJobs(const std::list<URL>& jobids) {

    logger.msg(DEBUG, "Removing jobs from job list and job store");

    FileLock lock(joblist);
    jobstorage.ReadFromFile(joblist);

    for (std::list<URL>::const_iterator it = jobids.begin();
         it != jobids.end(); it++) {

      XMLNodeList xmljobs = jobstorage.XPathLookup("//Job[JobID='" + it->str() + "']", NS());

      if (xmljobs.empty())
        logger.msg(ERROR, "Job %s has been deleted (i.e. was in job store), "
                   "but is not listed in job list", it->str());
      else {
        XMLNode& xmljob = *xmljobs.begin();
        if (xmljob) {
          logger.msg(DEBUG, "Removing job %s from job list file", it->str());
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

    jobstorage.SaveToFile(joblist);

    logger.msg(DEBUG, "Job store for %s now contains %d jobs", flavour, jobstore.size());
    logger.msg(DEBUG, "Finished removing jobs from job list and job store");

    return true;
  }

  std::list<Job> JobController::GetJobDescriptions(const std::list<std::string>& status,
                                                   const bool getlocal,
                                                   const int timeout) {

    logger.msg(DEBUG, "Getting %s jobs", flavour);
    GetJobInformation();

    // Only selected jobs with specified status
    std::list<Job> gettable;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++) {
      if (!status.empty() && it->State.empty()) {
        logger.msg(WARNING, "Job information not found: %s", it->JobID.str());
        continue;
      }

      if (!status.empty() && std::find(status.begin(), status.end(),
                                       it->State) == status.end())
        continue;
      gettable.push_back(*it);
    }

    //First try to get descriptions from local job file
    if (getlocal) {
      logger.msg(INFO, "Getting job decriptions from local job file");
      CheckLocalDescription(gettable);
    }
    else
      logger.msg(DEBUG, "Disregarding job decriptions from local job file");

    // Try to get description from cluster
    for (std::list<Job>::iterator it = gettable.begin();
         it != gettable.end();) {
      if (!it->JobDescription.empty()){
        it++;
        continue;
      }
      if (GetJobDescription(*it, it->JobDescription)) {
        logger.msg(INFO, "Got job description for %s", it->JobID.str());
        it++;
      } else {
        logger.msg(WARNING, "Failed getting job description for %s", it->JobID.str());
        it = gettable.erase(it);
      }
    }
    return gettable;

  }

  void JobController::CheckLocalDescription(std::list<Job>& jobs) {
    for (std::list<Job>::iterator it = jobs.begin();
         it != jobs.end();) {
      // Search for jobids
      XMLNodeList xmljobs =
        jobstorage.XPathLookup("//Job[JobID='" + it->JobID.str() + "']", NS());
      
      if (xmljobs.empty()) {
        logger.msg(ERROR, "Job not found in job list: %s", it->JobID.str());
        it++;
        continue;
      }
      XMLNode& xmljob = *xmljobs.begin();
      
      if (xmljob["JobDescription"]) {
        JobDescription jobdesc;
        jobdesc.setSource((std::string)xmljob["JobDescription"]);
        
        // Check for valid job description
        if (jobdesc.isValid())
          logger.msg(DEBUG, "Valid jobdescription found for: %s", it->JobID.str());
        else {
          logger.msg(WARNING, "No valid jobdescription found for: %s", it->JobID.str());
          it++;
          continue;
        }
        
        // Check checksums of local input files
        bool CKSUM = true;
        int size = xmljob["LocalInputFiles"].Size();
        for (int i = 0; i < size; i++) {
          const std::string file = (std::string)xmljob["LocalInputFiles"]["File"][i]["Source"];
          const std::string cksum_old = (std::string)xmljob["LocalInputFiles"]["File"][i]["CheckSum"];
          const std::string cksum_new = Sandbox::GetCksum(file);
          if (cksum_old != cksum_new) {
            logger.msg(WARNING, "Checksum of input file %s has changed.", file);
            CKSUM = false;
          } else
            logger.msg(DEBUG, "Stored and new checksum of input file %s are identical.", file);
        }
        // Push_back job and job descriptions
        if (CKSUM) {
          logger.msg(INFO, "Job description for %s retrieved locally", it->JobID.str());
          it->JobDescription = (std::string)xmljob["JobDescription"];
          it++;
        } else {
          logger.msg(WARNING, "Job %s can not be resubmitted", it->JobID.str());
          it = jobs.erase(it);
        }
      } else {
        logger.msg(WARNING, "Job description for %s could not be retrieved locally", it->JobID.str());
        it++;
      }
    } //end loop over jobs
    return;
  }
} // namespace Arc
