#include <arc/IString.h>
#include "JobController.h"
#include <arc/ArcConfig.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/FileLock.h>
#include <arc/XMLNode.h>
#include <arc/User.h>

#include <fstream>
#include <iostream>
#include <algorithm>

namespace Arc {

  Logger JobController::logger(Logger::getRootLogger(), "JobController");
  
  JobController::JobController(Arc::Config *cfg, std::string flavour)
    : ACC(), 
      GridFlavour(flavour){
    joblist = (std::string)(*cfg)["joblist"];
  }

  JobController::~JobController() {}
  
  void JobController::FillJobStore(std::list<std::string> jobs,
				   std::list<std::string> clusterselect,
				   std::list<std::string> clusterreject){
    
    mcfg.ReadFromFile(joblist);

    if(jobs.empty()){ //We are looking at all jobs of this grid flavour

      logger.msg(DEBUG, "Identifying all jobs of flavour %s", GridFlavour);	
      logger.msg(DEBUG, "Using joblist file %s", joblist);	

      Arc::XMLNodeList Jobs = mcfg.XPathLookup("//Job[flavour='"+ GridFlavour+"']", Arc::NS());
      XMLNodeList::iterator iter;
      
      for(iter = Jobs.begin(); iter!= Jobs.end(); iter++){
	Job ThisJob;
	ThisJob.JobID = (std::string) (*iter)["id"];
	ThisJob.InfoEndpoint = (std::string) (*iter)["source"];
	JobStore.push_back(ThisJob);
      }

    } else {//Jobs are targeted individually. The supplied jobs list may not contain any jobs of this grid flavour

      logger.msg(DEBUG, "Identifying individual jobs of flavour %s", GridFlavour);	
      logger.msg(DEBUG, "Using joblist file %s", joblist);	

      std::list<std::string>::iterator it;

      for(it = jobs.begin(); it != jobs.end(); it++){
	Arc::XMLNode ThisXMLJob = (*(mcfg.XPathLookup("//Job[id='"+ *it+"']", Arc::NS())).begin());
	if(GridFlavour == (std::string) ThisXMLJob["flavour"]){
	  Job ThisJob;
	  ThisJob.JobID = (std::string) ThisXMLJob["id"];
	  ThisJob.InfoEndpoint = (std::string) ThisXMLJob["source"];
	  JobStore.push_back(ThisJob);	  
	}
      }
    }

    logger.msg(DEBUG, "JobController%s has identified %d jobs", GridFlavour, JobStore.size());

  }

  void JobController::Get(const std::list<std::string>& jobs,
			  const std::list<std::string>& clusterselect,
			  const std::list<std::string>& clusterreject,
			  const std::list<std::string>& status,
			  const std::string downloaddir,
			  const bool keep,
			  const int timeout){

    logger.msg(DEBUG, "Getting %s jobs", GridFlavour);	
    std::list<std::string> JobsToBeRemoved;

    //Somewhere here add functionality for identifying jobs subject to the action (options -c -s etc)
    GetJobInformation();
    

    //loop over jobs
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){
      //first download job output
      bool downloaded = GetThisJob(*jobiter, downloaddir);
      if(!downloaded){
	std::cout<<Arc::IString("Failed downloading job %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      //second clean job (unless keep)
      if(!keep){
	JobsToBeRemoved.push_back(jobiter->JobID.str());
	bool cleaned = CleanThisJob(*jobiter, true);
	if(!cleaned){
	  std::cout<<Arc::IString("Failed cleaning job %s", jobiter->JobID.str())<<std::endl;
	  //continue ?
	}
      }
    } //end loop over jobs

    //Remove succesfully downloaded jobs
    RemoveJobs(JobsToBeRemoved);
    
  }

  void JobController::Kill(const std::list<std::string>& jobs,
			   const std::list<std::string>& clusterselect,
			   const std::list<std::string>& clusterreject,
			   const std::list<std::string>& status,
			   const bool keep,
			   const int timeout){
    
    logger.msg(DEBUG, "Killing %s jobs", GridFlavour);
    std::list<std::string> JobsToBeRemoved;

    //Somewhere here add functionality for identifying jobs subject to the action (options -c -s etc)
    GetJobInformation();

    //loop over jobs
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){
      //first cancel job (i.e. stop execution)
      bool cancelled = CancelThisJob(*jobiter);
      if(!cancelled){
	std::cout<<Arc::IString("Failed cancelling (stop) job %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      //second clean job (unless keep)
      if(!keep){
	JobsToBeRemoved.push_back(jobiter->JobID.str());
	bool cleaned = CleanThisJob(*jobiter, true);
	if(!cleaned){
	  std::cout<<Arc::IString("Failed cleaning job %s", jobiter->JobID.str())<<std::endl;
	  //continue ?
	}
      }
    } //end loop over jobs
    
    //Remove succesfully killed and cleaned jobs
    RemoveJobs(JobsToBeRemoved);

  }

  void JobController::Clean(const std::list<std::string>& jobs,
			    const std::list<std::string>& clusterselect,
			    const std::list<std::string>& clusterreject,
			    const std::list<std::string>& status,
			    const bool force,
			    const int timeout){
    
    logger.msg(DEBUG, "Cleaning %s jobs", GridFlavour);
    std::list<std::string> JobsToBeRemoved;
    
    //Somewhere here add functionality for identifying jobs subject to the action (options -c -s etc)
    GetJobInformation();

    //loop over jobs
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){
      bool cleaned = CleanThisJob(*jobiter, force);
      if(!cleaned){
	if(force)
	  JobsToBeRemoved.push_back(jobiter->JobID.str()); 
	std::cout<<Arc::IString("Failed cleaning job %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      JobsToBeRemoved.push_back(jobiter->JobID.str());       
    } //end loop over jobs

    //Remove succesfully killed and cleaned jobs
    RemoveJobs(JobsToBeRemoved);
    
  }

  void JobController::Cat(const std::list<std::string>& jobs,
			  const std::list<std::string>& clusterselect,
			  const std::list<std::string>& clusterreject,
			  const std::list<std::string>& status,
			  const std::string whichfile,
			  const int timeout){

    logger.msg(DEBUG, "Performing the 'cat' command on %s jobs", GridFlavour);

    //Somewhere here add functionality for identifying jobs subject to the action (options -c -s etc)
    GetJobInformation();
    
    //loop over jobs
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){
      if(jobiter->State.empty()){
	std::cout<<Arc::IString("Job state information not found: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      if(whichfile == "stdout" || whichfile == "stderr"){
	if(jobiter->State == "DELETED"){
	std::cout<<Arc::IString("Job has already been deleted: %s", jobiter->JobID.str())<<std::endl;	  
	continue;
	}
	if(jobiter->State == "ACCEPTING" || jobiter->State == "ACCEPTED" ||
	   jobiter->State == "PREPARING" || jobiter->State == "PREPARED" ||
	   jobiter->State == "INLRMS:Q") {
	  std::cout<<Arc::IString("Job has not started yet: %s", jobiter->JobID.str())<<std::endl;	  
	  continue;
	}
      }
      if(whichfile == "stdout" && jobiter->StdOut.empty()){
	std::cout<<Arc::IString("Can not determine the stdout location: %s", jobiter->JobID.str())<<std::endl;	  
	continue;
      }
      if(whichfile == "stderr" && jobiter->StdErr.empty()){
	std::cout<<Arc::IString("Can not determine the stderr location: %s", jobiter->JobID.str())<<std::endl;	  
	continue;
      }
      
      //get the file (stdout, stderr or gmlog)
      URL src = GetFileUrlThisJob((*jobiter), whichfile);
      URL dst("/tmp/arccat.XXXXXX");
      bool copied = CopyFile(src, dst);

      //output to screen
      if(copied){
	std::cout<<Arc::IString("%s from job %s", whichfile, jobiter->JobID.str())<<std::endl;
	char str[2000];
        
	std::fstream file("/tmp/arccat.XXXXXX", std::fstream::in);
        while(!file.eof()) {
	  file.getline(str,2000);
	  std::cout << str;
        }         
	file.close();
	std::cout <<std::endl;
      }

    } //end loop over jobs

  }

  void JobController::Stat(const std::list<std::string>& jobs,
			   const std::list<std::string>& clusterselect,
			   const std::list<std::string>& clusterreject,
			   const std::list<std::string>& status,
			   const bool longlist,
			   const int timeout){
    
    //Somewhere here add functionality for identifying jobs subject to the action (options -c -s etc)
    GetJobInformation();

    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){
      
      std::cout<<Arc::IString("Job: %s", jobiter->JobID.str())<<std::endl;
      if (!jobiter->Name.empty())
	std::cout<<Arc::IString(" Name: %s", jobiter->Name)<<std::endl;
      if (!jobiter->State.empty())
	std::cout<<Arc::IString(" State: %s", jobiter->State)<<std::endl;
      if(jobiter->ExitCode != -1)
	std::cout<<Arc::IString(" Exit Code: %d", jobiter->ExitCode)<<std::endl;
      if(!jobiter->Error.empty()){
	std::list<std::string>::iterator iter;
	for(iter = jobiter->Error.begin(); iter != jobiter->Error.end(); iter++ ){
	  std::cout<<Arc::IString(" Error: %s", *iter)<<std::endl;
	}
      }
      if (longlist) {
	if (!jobiter->Owner.empty())
	  std::cout<<Arc::IString(" Owner: %s", jobiter->Owner)<<std::endl;	  
	if (!jobiter->OtherMessages.empty())
	  std::cout<<Arc::IString(" Other Messages: %s", jobiter->OtherMessages)<<std::endl;	  	    
	if (!jobiter->ExecutionCE.empty())
	  std::cout<<Arc::IString(" ExecutionCE: %s", jobiter->ExecutionCE)<<std::endl;	  	      
	if (!jobiter->Queue.empty())
	  std::cout<<Arc::IString(" Queue: %s", jobiter->Queue)<<std::endl;	  	  
	if (jobiter->UsedSlots != -1)
	  std::cout<<Arc::IString(" Used Slots: %d", jobiter->UsedSlots)<<std::endl;	  
	if (jobiter->WaitingPosition != -1)
	  std::cout<<Arc::IString(" Waiting Position: %d", jobiter->WaitingPosition)<<std::endl;	                    
	if (!jobiter->StdIn.empty())
	  std::cout<<Arc::IString(" Stdin: %s", jobiter->StdIn)<<std::endl;	                                        
	if (!jobiter->StdOut.empty())
	  std::cout<<Arc::IString(" Stdout: %s", jobiter->StdOut)<<std::endl;	                                          
	if (!jobiter->StdErr.empty())
	  std::cout<<Arc::IString(" Stderr: %s", jobiter->StdErr)<<std::endl;	                    			  
	if (!jobiter->LogDir.empty())
	  std::cout<<Arc::IString(" Grid Manager Log Directory: %s", jobiter->LogDir)<<std::endl;
	if (jobiter->SubmissionTime != -1)
	  std::cout<<Arc::IString(" Submitted: %s", (std::string) jobiter->SubmissionTime)<<std::endl;
	if (jobiter->EndTime != -1)
	  std::cout<<Arc::IString(" End Time: %s", (std::string) jobiter->EndTime)<<std::endl;
	if (!jobiter->SubmissionHost.empty())
	  std::cout<<Arc::IString(" Submitted from: %s", jobiter->SubmissionHost)<<std::endl;                                  
	if (!jobiter->SubmissionClientName.empty())
	  std::cout<<Arc::IString(" Submitting client: %s", jobiter->SubmissionClientName)<<std::endl;
	if (jobiter->RequestedTotalCPUTime != -1)
	  std::cout<<Arc::IString(" Requested CPU Time: %s", (std::string) jobiter->RequestedTotalCPUTime)<<std::endl;
	if (jobiter->UsedTotalCPUTime != -1)
	  std::cout<<Arc::IString(" Used CPU Time: %s", (std::string) jobiter->UsedTotalCPUTime)<<std::endl;
	if (jobiter->UsedWallTime != -1)
	  std::cout<<Arc::IString(" Used Wall Time: %s", (std::string) jobiter->UsedWallTime)<<std::endl;
	if (jobiter->UsedMainMemory != -1)
	  std::cout<<Arc::IString(" Used Memory: %d", jobiter->UsedMainMemory)<<std::endl;
	if (jobiter->WorkingAreaEraseTime != -1)
	  std::cout<<Arc::IString((jobiter->State == "DELETED") ? 
				  " Results were deleted: %s" : 
				  " Results must be retrieved before: %s", 
				  (std::string) jobiter->WorkingAreaEraseTime)<<std::endl;
	if (jobiter->ProxyExpirationTime != -1)
	  std::cout<<Arc::IString(" Proxy valid until: %s", (std::string) jobiter->ProxyExpirationTime)<<std::endl;
	if (jobiter->CreationTime != -1)
	  std::cout<<Arc::IString(" Entry valid from: %s", (std::string) jobiter->CreationTime)<<std::endl;
	if (jobiter->Validity != -1)
	  std::cout<<Arc::IString(" Entry valid until: %s", (std::string) jobiter->Validity)<<std::endl;
      }//end if long
      
      std::cout<<std::endl;
      
    } //end loop over jobs
    
  }
  
  static void progress(FILE *o, const char *, unsigned int,
		       unsigned long long int all, unsigned long long int max,
		       double, double) {
    static int rs = 0;
    const char rs_[4] = {'|', '/', '-', '\\'};
    if (max) {
      fprintf(o, "\r|");
      unsigned int l = (74 * all + 37) / max;
      if (l > 74) l = 74;
      unsigned int i = 0;
      for (; i < l; i++) fprintf(o, "=");
      fprintf(o, "%c", rs_[rs++]);
      if (rs > 3) rs = 0;
      for (; i < 74; i++) fprintf(o, " ");
      fprintf(o, "|\r");
      fflush(o);
      return;
    }
    fprintf(o, "\r%llu kB                    \r", all / 1024);
  }

  bool JobController::CopyFile(URL src, URL dst){

    Arc::DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(true);
    mover.set_progress_indicator(&progress);
            
    logger.msg(DEBUG, "Now copying (from -> to)");
    logger.msg(DEBUG, " %s -> %s)", src.str(), dst.str());

    Arc::DataHandle source(src);
    if(!source){
      std::cout<<Arc::IString("Failed to get DataHandle on source: %s", src)<<std::endl;
      return false;
    }
    Arc::DataHandle destination(dst);
    if(!destination){
      std::cout<<Arc::IString("Failed to get DataHandle on destination: %s", destination)<<std::endl;
      return false;
    }
    
    // Create cache
    // This should not be needed (DataMover bug)
    Arc::User cache_user;
    std::string cache_path2;
    std::string cache_data_path;
    std::string id = "<ngcp>";
    Arc::DataCache cache(cache_path2, cache_data_path, "", id, cache_user);
    
    std::string failure;
    int timeout = 10;
    if (!mover.Transfer(*source, *destination, cache, Arc::URLMap(), 0, 0, 0, timeout, failure)) {
      if (!failure.empty()){
	std::cout<<Arc::IString("File download failed: %s", failure)<<std::endl;
      }else{
	std::cout<<Arc::IString("File download failed")<<std::endl;
      }
      return false;
    }

    return true;

  } //end CopyFile

  //Delete jobs from both joblist file and local JobStore
  void JobController::RemoveJobs(std::list<std::string> jobs){

    logger.msg(DEBUG, "Removing jobs from joblist and jobstore");	    

    //lock Joblist file
    Arc::FileLock lock(joblist);
    mcfg.ReadFromFile(joblist);
    
    std::list<std::string>::iterator it;
    
    //Identify jobs in joblist and remove from list
    for(it = jobs.begin(); it != jobs.end(); it++){
      Arc::XMLNode ThisXMLJob = (*(mcfg.XPathLookup("//Job[id='"+ (*it)+"']", Arc::NS())).begin());
      if(ThisXMLJob){
	logger.msg(DEBUG, "Removing job %s from joblist file", (*it));	
	ThisXMLJob.Destroy();
      }else {
	logger.msg(ERROR, "Job %s has been delete (i.e. was in JobStore), but is not listed in joblist", (*it));	
      }
      //Remove job from JobStore
      std::list<Job>::iterator jobiter = JobStore.begin();
      while(jobiter != JobStore.end()){
	if(jobiter->JobID.str() == (*it)){
	  JobStore.erase(jobiter);
	  break;
	}
	jobiter++;
      }
    }

    //write new joblist to file
    mcfg.SaveToFile(joblist);

    logger.msg(DEBUG, "JobStore%s now contains %d jobs", GridFlavour, JobStore.size());	    
    logger.msg(DEBUG, "Finished removing jobs from joblist and jobstore");	    
    
  }//end RemoveJobs

} // namespace Arc
