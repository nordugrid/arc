#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>
#include <stdio.h>
#include <iostream>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/StringConv.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetGenerator.h>
#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcstat");

void arcstat(const std::list<std::string>& jobs,
             const std::list<std::string>& clusterselect,
             const std::list<std::string>& clusterreject,
             const std::list<std::string>& status,
             const std::list<std::string>& giisurls,
	     const std::string joblist,
             const bool clusters,
             const bool longlist,
             const int timeout){
  
  if (clusters){ //i.e we are looking for queue or cluster info, not jobs
    
    //retrieve information
    Arc::TargetGenerator TarGen(clusterselect, clusterreject, giisurls);
    TarGen.GetTargets(0, 1);
    
    //print information to screen
    TarGen.PrintTargetInfo(longlist);
    
  } //end if clusters
    
  else { //i.e we are looking for the status of jobs

    Arc::JobSupervisor JobMaster(joblist, jobs);
    JobMaster.GetJobInformation();
    JobMaster.PrintJobInformation(longlist);

  } //end if we are looking for status of jobs

}

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[jobid]"), "",istring( 	  
			    "Argument to -g has format:\n"
			    "GRID:URL e.g. \n"
			    "ARC0:ldap://grid.tsl.uu.se:2135/mds-vo-name=sweden,O=grid\n"
			    "CREAM:ldap://cream.grid.upjs.sk:2170/o=grid\n"
			    "\n"
			    "Argument to -c has format:\n"
			    "GRID:URL e.g.\n"
			    "ARC0:ldap://grid.tsl.uu.se:2135/nordugrid-cluster-name=grid.tsl.uu.se,Mds-Vo-name=local,o=grid"));

  bool all = false;
  options.AddOption('a', "all",
		    istring("all jobs"),
		    all);

  std::string joblist;
  options.AddOption('j', "joblist", istring("file containing a list of jobids"), 
		    istring("filename"),
		    joblist);

  std::list<std::string> clustertemp;
  options.AddOption('c', "cluster", istring("explicity select or reject a specific cluster"), 
		    istring("[-]name"),
		    clustertemp);

  std::list<std::string> status;
  options.AddOption('s', "status", istring("only select jobs whose status is statusstr"), 
		    istring("statustr"),
		    status);

  std::list<std::string> indexurls;
  options.AddOption('i', "indexurl", istring("url to a index server"), 
		    istring("url"),
		    indexurls);

  bool clusters = false;
  options.AddOption('q', "queues",
		    istring("show information about clusters and queues"),
		    clusters);

  bool longlist = false;
  options.AddOption('l', "long",
		    istring("long format (more information)"),
		    longlist);

  int timeout = 20;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
		    istring("seconds"), timeout);  
  
  std::string debug;
  options.AddOption('d', "debug",
		    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
		    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
		    version);

  std::list<std::string> params = options.Parse(argc, argv);

  //sort clustertemp into clusterselect and clusterreject
  std::list<std::string> clusterselect;
  std::list<std::string> clusterreject;
  for(std::list<std::string>::iterator it = clustertemp.begin(); it != clustertemp.end();it++){
    if((*it).find_first_of('-') == 0){
      clusterreject.push_back(*it);
    } else{
      clusterselect.push_back(*it);
    }
  }

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcstat", VERSION) << std::endl;
    return 0;
  }

  if (!params.empty() && all) {
    logger.msg(Arc::ERROR, "Option -a [-all] can not be used together with jobid");
    return 1;
  }

  arcstat(params, clusterselect, clusterreject, status, indexurls, joblist, 
	  clusters, longlist, timeout);

  return 0;

}
