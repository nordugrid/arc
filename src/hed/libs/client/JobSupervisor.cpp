#include <algorithm>
#include <iostream>

#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/StringConv.h>
#include <arc/loader/Loader.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/ClientInterface.h>

namespace Arc {
  
  JobSupervisor::JobSupervisor(std::string joblist, std::list<std::string> jobids) {

    XMLNode JobIdStorage;    
    if(!joblist.empty()){
      JobIdStorage.ReadFromFile(joblist);
    } else{
      JobIdStorage.ReadFromFile("jobs.xml");
    }

    std::list<std::string> NeededControllers;
    
    Arc::XMLNodeList ActiveJobs = JobIdStorage.XPathLookup("/jobs/job", Arc::NS());  
    Arc::XMLNodeList::iterator JobIter;
    
    for(JobIter = ActiveJobs.begin(); JobIter != ActiveJobs.end(); JobIter++){
      if(std::find(NeededControllers.begin(), NeededControllers.end(), (std::string) (*JobIter)["flavour"]) == NeededControllers.end()){
	std::cout<<"Need JobController for grid flavour: "<<(std::string) (*JobIter)["flavour"] << std::endl;	
	NeededControllers.push_back((std::string) (*JobIter)["flavour"]);
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
      ThisJobController.NewChild(JobIdStorage);
      JobControllerNumber++;
      
    }
    
    ACCloader = new Loader(&mcfg);

    for(int i = 1; i < JobControllerNumber; i++){
      JobController *JC = dynamic_cast<JobController*> (ACCloader->getACC("controller"+Arc::tostring(i)));
      if(JC) {
	JobControllers.push_back(JC);
	(*JobControllers.rbegin())->IdentifyJobs(jobids);
      }
    }
    
    std::cout<<"Number of JobControllers: "<<JobControllers.size()<<std::endl;        

  }
  
  JobSupervisor::~JobSupervisor() {
    
    if (ACCloader)
      delete ACCloader;
  }

  void JobSupervisor::GetJobInformation() {

    std::list<JobController*>::iterator iter;
    
    //This may benefit from being threaded
    for(iter = JobControllers.begin(); iter != JobControllers.end(); iter++){
      (*iter)->GetJobInformation();
    }

  }


  void JobSupervisor::PrintJobInformation(bool longlist) {

    std::list<JobController*>::iterator iter;
    
    //This may benefit from being threaded
    for(iter = JobControllers.begin(); iter != JobControllers.end(); iter++){
      (*iter)->PrintJobInformation(longlist);
    }
    
  }

} // namespace Arc
