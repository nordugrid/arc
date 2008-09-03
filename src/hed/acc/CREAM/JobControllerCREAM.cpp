#include <arc/URL.h>
#include <arc/message/MCC.h>

#include "CREAMClient.h"
#include "JobControllerCREAM.h"

namespace Arc {

  Logger JobControllerCREAM::logger(JobController::logger, "CREAM");

  JobControllerCREAM::JobControllerCREAM(Config *cfg)
    : JobController(cfg, "CREAM") {}

  JobControllerCREAM::~JobControllerCREAM() {}

  ACC *JobControllerCREAM::Instance(Config *cfg, ChainContext*) {
    return new JobControllerCREAM(cfg);
  }

  void JobControllerCREAM::GetJobInformation() {
    for (std::list<Job>::iterator iter = JobStore.begin();
	 iter != JobStore.end(); iter++) {
      MCCConfig cfg;
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      Cream::CREAMClient gLiteClient(url, cfg);
      if (!gLiteClient.stat(pi.Rest(), iter->State)) {
	logger.msg(ERROR, "Could not retrieve job information");
      }
    }
  }

  bool JobControllerCREAM::GetThisJob(Job ThisJob,
				      const std::string& downloaddir) {

    logger.msg(DEBUG, "Downloading job: %s", ThisJob.InfoEndpoint.str());
    bool SuccessfulDownload = true;

    DataHandle source(ThisJob.InfoEndpoint);    
    if (source) {

      std::list<std::string> downloadthese = GetDownloadFiles(source);

      //loop over files
      for(std::list<std::string>::iterator i = downloadthese.begin();
	  i != downloadthese.end(); i++) {
	std::string src = ThisJob.InfoEndpoint.str() + "/"+ *i;
	std::string path_temp = ThisJob.JobID.Path(); 
	size_t slash = path_temp.find_last_of("/");
	std::string dst;
	if(downloaddir.empty())
	  dst = path_temp.substr(slash+1) + "/" + *i;
	else
	  dst = downloaddir + "/" + *i;
	bool GotThisFile = CopyFile(src, dst);
	if(!GotThisFile)
	  SuccessfulDownload = false;
      }
    }
    else
      logger.msg(ERROR, "Failed dowloading job: %s. "
		 "Could not get data handle.", ThisJob.InfoEndpoint.str());
  }

  bool JobControllerCREAM::CleanThisJob(Job ThisJob, bool force) {

    MCCConfig cfg;
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    Cream::CREAMClient gLiteClient(url, cfg);
    if (!gLiteClient.purge(pi.Rest())) {
      logger.msg(ERROR, "Failed to clean job");
      return false;
    }
    return true;
  }

  bool JobControllerCREAM::CancelThisJob(Job ThisJob){

    MCCConfig cfg;
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    Cream::CREAMClient gLiteClient(url, cfg);
    if (!gLiteClient.cancel(pi.Rest())) {
      logger.msg(ERROR, "Failed to cancel job");
      return false;
    }
    return true;
  }

  URL JobControllerCREAM::GetFileUrlThisJob(Job ThisJob,
					    const std::string& whichfile){};

  std::list<std::string>
  JobControllerCREAM::GetDownloadFiles(DataHandle& dir,
				       const std::string& dirname) {

    std::list<std::string> files;

    std::list<FileInfo> outputfiles;
    dir->ListFiles(outputfiles, true);

    for(std::list<FileInfo>::iterator i = outputfiles.begin();
	i != outputfiles.end(); i++) {
      if(i->GetType() == 0 || i->GetType() == 1) {
	if(!dirname.empty())
	  files.push_back(dirname + "/" + i->GetName());
	else
	  files.push_back(i->GetName());
      }
      else if(i->GetType() == 2) {
	DataHandle tmpdir(dir->str() + "/" + i->GetName());
	std::list<std::string> morefiles = GetDownloadFiles(tmpdir,
							    i->GetName());
	for(std::list<std::string>::iterator j = morefiles.begin();
	    j != morefiles.end(); j++)
	  if(!dirname.empty())
	    files.push_back(dirname + "/"+ *j);
	  else
	    files.push_back(*j);
      }
    }
    return files;
  }

} // namespace Arc
