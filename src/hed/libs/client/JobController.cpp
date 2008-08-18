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

    if(!jobs.empty()){

      logger.msg(DEBUG, "Filling JobStore with jobs according to specified jobids");	
      logger.msg(DEBUG, "Using joblist file %s", joblist);	
      
      std::list<std::string>::iterator it;
      
      for(it = jobs.begin(); it != jobs.end(); it++){
	Arc::XMLNode ThisXMLJob = (*(mcfg.XPathLookup("//Job[id='"+ *it+"']", Arc::NS())).begin());
	if(GridFlavour == (std::string) ThisXMLJob["flavour"]){
	  Job ThisJob;
	  ThisJob.JobID = (std::string) ThisXMLJob["id"];
	  ThisJob.InfoEndpoint = (std::string) ThisXMLJob["source"];
	  ThisJob.Cluster = (std::string) ThisXMLJob["cluster"];
	  ThisJob.LocalSubmissionTime = (std::string) ThisXMLJob["submissiontime"];
	  JobStore.push_back(ThisJob);	  
	}
      }
    } // end if jobids are given

    if(!clusterselect.empty()){
      
      logger.msg(DEBUG, "Filling JobStore with jobs according to selected clusters");	
      logger.msg(DEBUG, "Using joblist file %s", joblist);	
     
      Arc::XMLNodeList Jobs = mcfg.XPathLookup("//Job[flavour='"+ GridFlavour+"']", Arc::NS());
      XMLNodeList::iterator iter;
      
      for(iter = Jobs.begin(); iter!= Jobs.end(); iter++){

	URL ThisJobRunsOn = (std::string) (*iter)["cluster"];

	if(PresentInList(ThisJobRunsOn, clusterselect)){
	  Job ThisJob;
	  ThisJob.JobID = (std::string) (*iter)["id"];
	  ThisJob.InfoEndpoint = (std::string) (*iter)["source"];
	  ThisJob.Cluster = (std::string) (*iter)["cluster"];
	  ThisJob.LocalSubmissionTime = (std::string) (*iter)["submissiontime"];
	  JobStore.push_back(ThisJob);
	}
      }
    } //end if clusters are selected

    if(!clusterreject.empty()){
      if(!JobStore.empty()){
	
	logger.msg(DEBUG, "Removing from JobStore jobs according to cluster reject list");	
	logger.msg(DEBUG, "Using joblist file %s", joblist);	
	
	std::list<Job>::iterator iter = JobStore.begin();
	while(iter!= JobStore.end()){
	  if(PresentInList(iter->Cluster, clusterreject)){
	    logger.msg(DEBUG, "Removing Job %s from JobStore as it runs on a rejected cluster", iter->JobID.str());
	    JobStore.erase(iter);
	    continue;
	  }
	  iter++;
	}
      }
    } // end if cluster reject is given

    if(jobs.empty() && clusterselect.empty()){ // add all
      
      logger.msg(DEBUG, "Filling JobStore with all jobs, except those running on rejected clusters");	
      logger.msg(DEBUG, "Using joblist file %s", joblist);	
      
      Arc::XMLNodeList Jobs = mcfg.XPathLookup("//Job[flavour='"+ GridFlavour+"']", Arc::NS());
      XMLNodeList::iterator iter;
      
      for(iter = Jobs.begin(); iter!= Jobs.end(); iter++){

	URL ThisJobRunsOn = (std::string) (*iter)["cluster"];

	if(!PresentInList(ThisJobRunsOn, clusterreject)){
	  Job ThisJob;
	  ThisJob.JobID = (std::string) (*iter)["id"];
	  ThisJob.InfoEndpoint = (std::string) (*iter)["source"];
	  ThisJob.Cluster = (std::string) (*iter)["cluster"];
	  ThisJob.LocalSubmissionTime = (std::string) (*iter)["submissiontime"];
	  JobStore.push_back(ThisJob);
	}
      }
    } // end adding all jobs except those running on rejected clusters
    
    logger.msg(DEBUG, "FillJobStore has finished succesfully");
    logger.msg(DEBUG, "JobStore%s contains %d jobs", GridFlavour, JobStore.size());
    
  } // end FillJobStore

  void JobController::Get(const std::list<std::string>& jobs,
			  const std::list<std::string>& clusterselect,
			  const std::list<std::string>& clusterreject,
			  const std::list<std::string>& status,
			  const std::string downloaddir,
			  const bool keep,
			  const int timeout){

    logger.msg(DEBUG, "Getting %s jobs", GridFlavour);	
    std::list<std::string> JobsToBeRemoved;

    //First get information about the selected jobs
    GetJobInformation();

    //Second, filter them according to their state (i.e. which states we can download)
    std::list<Job*> Downloadable;
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){

      if (jobiter->State.empty()){
	std::cout<<Arc::IString("Job information not found: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      
      if (!status.empty() && std::find(status.begin(), status.end(), jobiter->State) == status.end())
	continue;
      
      if ( jobiter->State == "DELETED") {
	std::cout<<Arc::IString("Job has already been deleted: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      
      if (jobiter->State != "FINISHED" && jobiter->State != "FAILED" && jobiter->State != "KILLED"){
	std::cout<<Arc::IString("Job has not finished yet: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      
      Downloadable.push_back(&(*jobiter));

    }

    //loop over jobs
    for(std::list<Arc::Job*>::iterator jobiter = Downloadable.begin(); jobiter!= Downloadable.end(); jobiter++){    
      //first download job output
      bool downloaded = GetThisJob(**jobiter, downloaddir);
      if(!downloaded){
	std::cout<<Arc::IString("Failed downloading job %s", (*jobiter)->JobID.str())<<std::endl;
	continue;
      }
      //second clean job (unless keep)
      if(!keep){
	bool cleaned = CleanThisJob(**jobiter, true);
	if(!cleaned){
	  std::cout<<Arc::IString("Failed cleaning job %s", (*jobiter)->JobID.str())<<std::endl;
	  continue;
	}
	JobsToBeRemoved.push_back((*jobiter)->JobID.str());
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
    
    //First get information about the selected jobs
    GetJobInformation();

    //Second, filter them according to their state (i.e. which states we can kill)
    std::list<Job*> Killable;
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){

      if (jobiter->State.empty()){
	std::cout<<Arc::IString("Job information not found: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      
      if (!status.empty() && std::find(status.begin(), status.end(), jobiter->State) == status.end())
	continue;
      
      if ( jobiter->State == "DELETED") {
	std::cout<<Arc::IString("Job has already been deleted: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      
      if (jobiter->State == "FINISHED" && jobiter->State == "FAILED" && jobiter->State == "KILLED"){
	std::cout<<Arc::IString("Job has already finished: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      
      Killable.push_back(&(*jobiter));

    }

    //loop over jobs
    for(std::list<Arc::Job*>::iterator jobiter = Killable.begin(); jobiter!= Killable.end(); jobiter++){
      //first cancel job (i.e. stop execution)
      bool cancelled = CancelThisJob(**jobiter);
      if(!cancelled){
	std::cout<<Arc::IString("Failed cancelling (stop) job %s", (*jobiter)->JobID.str())<<std::endl;
	continue;
      }
      //second clean job (unless keep)
      if(!keep){
	bool cleaned = CleanThisJob(**jobiter, true);
	if(!cleaned){
	  std::cout<<Arc::IString("Failed cleaning job %s", (*jobiter)->JobID.str())<<std::endl;
	  continue;
	}
	JobsToBeRemoved.push_back((*jobiter)->JobID.str());
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
    
    //First get information about the selected jobs
    GetJobInformation();
    
    //Second, filter them according to their state (i.e. which states we can clean)
    std::list<Job*> Cleanable;
    for(std::list<Arc::Job>::iterator jobiter = JobStore.begin(); jobiter!= JobStore.end(); jobiter++){

      if (force && jobiter->State.empty() && status.empty()){
	std::cout<<Arc::IString("Job %s will only be deleted from local joblist", jobiter->JobID.str())<<std::endl;
	JobsToBeRemoved.push_back(jobiter->JobID.str());
	continue;
      }
      
      if (jobiter->State.empty()){
	std::cout<<Arc::IString("Job information not found: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }      
      
      if (!status.empty() && std::find(status.begin(), status.end(), jobiter->State) == status.end())
	continue;
      
      
      if (jobiter->State != "FINISHED" && jobiter->State != "FAILED" && 
	  jobiter->State != "KILLED" && jobiter->State != "DELETED"){
	std::cout<<Arc::IString("Job has not finished yet: %s", jobiter->JobID.str())<<std::endl;
	continue;
      }
      
      Cleanable.push_back(&(*jobiter));

    }

    //loop over jobs
    for(std::list<Arc::Job*>::iterator jobiter = Cleanable.begin(); jobiter!= Cleanable.end(); jobiter++){
      bool cleaned = CleanThisJob(**jobiter, force);
      if(!cleaned){
	if(force)
	  JobsToBeRemoved.push_back((*jobiter)->JobID.str()); 
	std::cout<<Arc::IString("Failed cleaning job %s", (*jobiter)->JobID.str())<<std::endl;
	continue;
      }
      JobsToBeRemoved.push_back((*jobiter)->JobID.str());       
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

    //First get information about the selected jobs
    GetJobInformation();
    
    //Second, filter them according to their state (i.e. which states we can clean)
    std::list<Job*> Catable;
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

      Catable.push_back(&(*jobiter));
      
    }


    //loop over jobs
    for(std::list<Arc::Job*>::iterator jobiter = Catable.begin(); jobiter!= Catable.end(); jobiter++){
      
      //get the file (stdout, stderr or gmlog)
      URL src = GetFileUrlThisJob((**jobiter), whichfile);
      URL dst("/tmp/arccat.XXXXXX");
      bool copied = CopyFile(src, dst);

      //output to screen
      if(copied){
	std::cout<<Arc::IString("%s from job %s", whichfile, (*jobiter)->JobID.str())<<std::endl;
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
      if(jobiter->State.empty()){
	std::cout<<Arc::IString("Job state information not found: %s", jobiter->JobID.str())<<std::endl;
	Time now;
	if(now - jobiter->LocalSubmissionTime < 90){
	  std::cout<<Arc::IString("This job was very recently "
				  "submitted and might not yet" 
				  "have reached the information-system")<<std::endl;
	}
	continue;
      }
      jobiter->Print(longlist);
    }
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
      std::cout<<Arc::IString("Failed to get DataHandle on source: %s", src.str())<<std::endl;
      return false;
    }
    Arc::DataHandle destination(dst);
    if(!destination){
      std::cout<<Arc::IString("Failed to get DataHandle on destination: %s", dst.str())<<std::endl;
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


  //Present implementation not ideal for later GUI usage, i.e. does not make use of jobs, clusterselect, clusterreject 
  //(we know that the JobStore is only filled with jobs according to these criteria)
  //(note for later: GetJobInformation should be modified to take jobs, clusterselect, clusterreject, status as input)
  /*
  std::list<Job*> JobController::IdentifyJobs(jobs, clusterselect, clusterreject, status){
    GetJobInformation();    
  }
  */

  bool JobController::PresentInList(URL url, std::list<std::string> searchurl_strings){
    bool answer(true);
    std::list<URL> SearchUrls;
    
    //First, resolve aliases and prepare list of URLs to search within
    for(std::list<std::string>::iterator iter  = searchurl_strings.begin(); iter != searchurl_strings.end(); iter++){
      //check if alias (not yet implemented)
      SearchUrls.push_back(*iter);
    }

    if(std::find(SearchUrls.begin(),SearchUrls.end(), url) == SearchUrls.end())
      answer = false;

    return answer;
 }

} // namespace Arc
