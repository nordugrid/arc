#include <list>
#include <string>
#include <stdio.h>
#include <iostream>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/StringConv.h>
#include <arc/loader/Loader.h>
#include <arc/misc/ClientInterface.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetGenerator.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcstat");

void arcstat(const std::list<std::string>& jobs,
             const std::list<std::string>& clusterselect,
             const std::list<std::string>& clusterreject,
             const std::list<std::string>& status,
             const std::list<std::string>& giisurls,
             const bool clusters,
             const bool longlist,
             const int timeout,
             const bool anonymous){
  
  if (clusters){ //i.e we are looking for queue or cluster info, not jobs
    
    Arc::ACCConfig acccfg;
    Arc::NS ns;
    Arc::Config mcfg(ns);
    acccfg.MakeConfig(mcfg);
    
    bool ClustersSpecified = false;
    bool IndexServersSpecified = false;
    int TargetURL = 1;

    //first add to config element the specified target clusters (if any)
    for (std::list<std::string>::const_iterator it = clusterselect.begin(); it != clusterselect.end(); it++){
      
      size_t colon = (*it).find_first_of(":");
      std::string GridFlavour = (*it).substr(0, colon);
      std::string SomeURL = (*it).substr(colon+1);

      Arc::XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
      ThisRetriever.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
      ThisRetriever.NewAttribute("id") = "retriever" + Arc::tostring(TargetURL);
      Arc::XMLNode ThisRetriever1 = ThisRetriever.NewChild("URL") = (std::string) SomeURL;
      ThisRetriever1.NewAttribute("ServiceType") = "computing";

      TargetURL++;
      ClustersSpecified = true;
    }

    //if no cluster url are given next steps are index servers (giis'es in ARC0)
    if (!ClustersSpecified){ //means that -c option takes priority over -g
      for (std::list<std::string>::const_iterator it = giisurls.begin(); it != giisurls.end(); it++){      

	size_t colon = (*it).find_first_of(":");
	std::string GridFlavour = (*it).substr(0, colon);
	std::string SomeURL = (*it).substr(colon+1);
	
	Arc::XMLNode ThisRetriever = mcfg.NewChild("ArcClientComponent");
	ThisRetriever.NewAttribute("name") = "TargetRetriever"+ (std::string) GridFlavour;
	ThisRetriever.NewAttribute("id") = "retriever" + Arc::tostring(TargetURL);
	Arc::XMLNode ThisRetriever1 = ThisRetriever.NewChild("URL") = (std::string) SomeURL;
	ThisRetriever1.NewAttribute("ServiceType") = "index";
	
	TargetURL++;
	IndexServersSpecified = true;
      }
    }
    
    //if neither clusters nor index servers are specified, read from config file
    if(!ClustersSpecified && !IndexServersSpecified){
      
    }

    //remove cluster that are rejected by user
    for (std::list<std::string>::const_iterator it = clusterreject.begin();it != clusterreject.end(); it++){

    }
    
    //get cluster information
    Arc::TargetGenerator TarGen(mcfg);
    TarGen.GetTargets(0, 1);
    
    //finally print information to screen
    TarGen.PrintTargetInfo(longlist);
    
  } //end if clusters
    
  else { //i.e we are looking for the status of jobs

    Arc::XMLNode JobIdStorage;
    JobIdStorage.ReadFromFile("jobs.xml");
    
    Arc::JobSupervisor JobMaster(JobIdStorage, jobs);
    JobMaster.GetJobInformation();
    JobMaster.PrintJobInformation(longlist);

  } //end if we are looking for status of jobs

}

#define ARCSTAT
#include "arccli.cpp"
