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
    
    //retrieve information
    Arc::TargetGenerator TarGen(clusterselect, clusterreject, giisurls);
    TarGen.GetTargets(0, 1);
    
    //print information to screen
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
