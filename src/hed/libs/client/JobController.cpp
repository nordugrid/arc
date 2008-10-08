#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <fstream>
#include <iostream>

#include <unistd.h>

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/XMLNode.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/FileCache.h>
#include <arc/data/URLMap.h>

#include "JobController.h"

namespace Arc {

  Logger JobController::logger(Logger::getRootLogger(), "JobController");

  JobController::JobController(Config *cfg, const std::string& flavour)
    : ACC(cfg, flavour),
      joblist((*cfg)["JobList"]) {}

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
	  jobstore.push_back(job);
	}
      }
    }

    logger.msg(DEBUG, "FillJobStore has finished succesfully");
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
      int tmp_h = mkstemp(const_cast<char*>(filename.c_str()));
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
		     "submitted and might not yet"
		     "have reached the information-system");
	continue;
      }
      it->Print(longlist);
    }
    return true;
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
    logger.msg(DEBUG, " %s -> %s)", src.str(), dst.str());

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

      XMLNodeList xmljobs =
	jobstorage.XPathLookup("//Job[JobID='" + it->str() + "']", NS());

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

    logger.msg(DEBUG, "Job store for %s now contains %d jobs",
	       flavour, jobstore.size());
    logger.msg(DEBUG, "Finished removing jobs from job list and job store");

    return true;
  }

} // namespace Arc
