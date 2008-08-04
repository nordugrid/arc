#include <algorithm>
#include <iostream>

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  Logger JobSupervisor::logger(Logger::getRootLogger(), "JobSupervisor");
  
  JobSupervisor::JobSupervisor(const std::list<std::string>& jobs,
			       const std::list<std::string>& clusterselect,
			       const std::list<std::string>& clusterreject,
			       const std::list<std::string>& status,
			       const std::string downloaddir,
			       const std::string joblist,
			       const bool keep,
			       const bool force,
			       const int timeout) : 
			       clusterselect(clusterselect),
			       clusterreject(clusterreject),
			       status(status),
			       downloaddir(downloaddir),
			       keep(keep),
			       force(force),
			       timeout(timeout){
 
    Config JobIdStorage;
    if(!joblist.empty()){
      FileLock lock(joblist);
      JobIdStorage.ReadFromFile(joblist);
    }

    std::list<std::string> NeededControllers;

    //First, if jobids are specified, only load JobControllers needed by those jobs
    if(!jobs.empty()){
      logger.msg(DEBUG, "Identifying needed JobControllers according to specified job ids");
      std::list<std::string>::const_iterator it;
      
      for(it = jobs.begin(); it != jobs.end(); it++){
	Arc::XMLNode ThisXMLJob = (*(JobIdStorage.XPathLookup("//Job[id='"+ *it+"']", Arc::NS())).begin());
	if(ThisXMLJob){
	  if (std::find(NeededControllers.begin(), NeededControllers.end(),
			(std::string) ThisXMLJob["flavour"]) == NeededControllers.end()){
	    std::string flavour = (std::string) ThisXMLJob["flavour"];
	    logger.msg(DEBUG, "Need jobController for grid flavour %s", flavour);	
	    NeededControllers.push_back(flavour);
	  }  
	} else{
	  std::cout<<Arc::IString(" Job Id = %s not found", (*it))<<std::endl;
	}
      }
    } else{ //load controllers for all grid flavours present in joblist

      logger.msg(DEBUG, "Identifying needed JobControllers according to all jobs present in joblist");

      Arc::XMLNodeList ActiveJobs =
	JobIdStorage.XPathLookup("/ArcConfig/Job", Arc::NS());
      
      for (Arc::XMLNodeList::iterator JobIter = ActiveJobs.begin();
	   JobIter != ActiveJobs.end(); JobIter++) {
	if (std::find(NeededControllers.begin(), NeededControllers.end(),
		      (std::string)(*JobIter)["flavour"]) == NeededControllers.end()){
	  std::string flavour = (*JobIter)["flavour"];
	  logger.msg(DEBUG, "Need jobController for grid flavour %s", flavour);	
	  NeededControllers.push_back((std::string)(*JobIter)["flavour"]);
	}
      }
    
    }

    Arc::ACCConfig acccfg;
    Arc::NS ns;
    Arc::Config mcfg(ns);
    acccfg.MakeConfig(mcfg);
    
    std::list<std::string>::const_iterator iter;
    int JobControllerNumber = 1;
    
    for(iter = NeededControllers.begin(); iter != NeededControllers.end(); iter++){
      Arc::XMLNode ThisJobController = mcfg.NewChild("ArcClientComponent");
      ThisJobController.NewAttribute("name") = "JobController"+ (*iter);
      ThisJobController.NewAttribute("id") = "controller" + Arc::tostring(JobControllerNumber);
      ThisJobController.NewChild("joblist") = joblist;      
      JobControllerNumber++;
    }
    
    ACCloader = new Loader(&mcfg);

    for(int i = 1; i < JobControllerNumber; i++){
      JobController *JC = dynamic_cast<JobController*> (ACCloader->getACC("controller"+Arc::tostring(i)));
      if(JC) {
	JobControllers.push_back(JC);
	(*JobControllers.rbegin())->IdentifyJobs(jobs);
      }
    }
    
  }
  
  JobSupervisor::~JobSupervisor() {
    
    if (ACCloader)
      delete ACCloader;
  }

  void JobSupervisor::GetJobInformation() {

    if(JobControllers.empty())
      return;
    logger.msg(DEBUG, "Getting job information");	

    std::list<JobController*>::iterator iter;
    
    //This may benefit from being threaded
    for(iter = JobControllers.begin(); iter != JobControllers.end(); iter++){
      (*iter)->GetJobInformation();
    }
  }

  void JobSupervisor::DownloadJobOutput() {

    if(JobControllers.empty())
      return;
    logger.msg(DEBUG, "Downloading job output");	

    std::list<JobController*>::iterator iter;
    
    //This may benefit from being threaded
    for(iter = JobControllers.begin(); iter != JobControllers.end(); iter++){
      (*iter)->DownloadJobOutput(keep, downloaddir);
    }
  }


  void JobSupervisor::PrintJobInformation(bool longlist) {

    if(JobControllers.empty())
      return;
    logger.msg(DEBUG, "Printing job information");	

    std::list<JobController*>::iterator iter;
    
    for(iter = JobControllers.begin(); iter != JobControllers.end(); iter++){
      (*iter)->PrintJobInformation(longlist);
    }
  }

  void JobSupervisor::Clean() {

    if(JobControllers.empty())
      return;
    logger.msg(DEBUG, "Cleaning job(s)");	

    std::list<JobController*>::iterator iter;
    
    for(iter = JobControllers.begin(); iter != JobControllers.end(); iter++){
      (*iter)->Clean(force);
    }
  }

  void JobSupervisor::Kill() {

    if(JobControllers.empty())
      return;
    logger.msg(DEBUG, "Killing job(s)");	

    std::list<JobController*>::iterator iter;
    
    for(iter = JobControllers.begin(); iter != JobControllers.end(); iter++){
      (*iter)->Kill(keep);
    }
  }

} // namespace Arc
