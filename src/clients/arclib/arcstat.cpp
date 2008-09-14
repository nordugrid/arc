#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/UserConfig.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcstat");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[jobid ...]"), "",
			    istring("Argument to -i has format:\n"
				    "GRID:URL e.g.\n"
				    "ARC0:ldap://grid.tsl.uu.se:2135/"
				    "mds-vo-name=sweden,O=grid\n"
				    "CREAM:ldap://cream.grid.upjs.sk:2170/"
				    "o=grid\n"
				    "\n"
				    "Argument to -c has format:\n"
				    "GRID:URL e.g.\n"
				    "ARC0:ldap://grid.tsl.uu.se:2135/"
				    "nordugrid-cluster-name=grid.tsl.uu.se,"
				    "Mds-Vo-name=local,o=grid"));

  bool all = false;
  options.AddOption('a', "all",
		    istring("all jobs"),
		    all);

  std::string joblist;
  options.AddOption('j', "joblist",
		    istring("file containing a list of jobids"),
		    istring("filename"),
		    joblist);

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
		    istring("explicity select or reject a specific cluster"),
		    istring("[-]name"),
		    clusters);

  std::list<std::string> status;
  options.AddOption('s', "status",
		    istring("only select jobs whose status is statusstr"),
		    istring("statusstr"),
		    status);

  std::list<std::string> indexurls;
  options.AddOption('i', "indexurl", istring("url to a index server"),
		    istring("url"),
		    indexurls);

  bool queues = false;
  options.AddOption('q', "queues",
		    istring("show information about clusters and queues"),
		    queues);

  bool longlist = false;
  options.AddOption('l', "long",
		    istring("long format (more information)"),
		    longlist);

  int timeout = 20;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
		    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
		    istring("configuration file (default ~/.arc/client.xml)"),
		    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
		    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
		    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
		    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcstat", VERSION)
	      << std::endl;
    return 0;
  }

  std::list<Arc::URL> jobids;
  jobids.insert(jobids.end(), params.begin(), params.end());

  if (queues) {
    // i.e we are looking for queue or cluster info, not jobs
    Arc::TargetGenerator targen(usercfg, clusters, indexurls);
    targen.GetTargets(0, 1);
    targen.PrintTargetInfo(longlist);
    return 0;
  }

  else {
    // i.e we are looking for the status of jobs
    if (jobids.empty() && joblist.empty() && clusters.empty() && !all) {
      logger.msg(Arc::ERROR, "No valid jobids given");
      return 1;
    }

    if (joblist.empty())
      joblist = usercfg.JobListFile();

    Arc::JobSupervisor jobmaster(usercfg, jobids, clusters, joblist);

    std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();

    if (jobcont.empty()) {
      logger.msg(Arc::ERROR, "No job controllers loaded");
      return 1;
    }

    int retval = 0;
    for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
	 it != jobcont.end(); it++)
      if (!(*it)->Stat(jobids, status, longlist, timeout))
	retval = 1;

    return retval;
  }
}
