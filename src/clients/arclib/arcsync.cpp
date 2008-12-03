#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/FileLock.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/UserConfig.h>
#include <arc/client/ACC.h>
#include <arc/client/Broker.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsync");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[filename ...]"),
			    istring("The command is used for "
				    "jobid synchronization"),
			    istring("Argument to -i has the format "
				    "Flavour:URL e.g.\n"
				    "ARC0:ldap://grid.tsl.uu.se:2135/"
				    "mds-vo-name=sweden,O=grid\n"
				    "CREAM:ldap://cream.grid.upjs.sk:2170/"
				    "o=grid\n"
				    "\n"
				    "Argument to -c has the format "
				    "Flavour:URL e.g.\n"
				    "ARC0:ldap://grid.tsl.uu.se:2135/"
				    "nordugrid-cluster-name=grid.tsl.uu.se,"
				    "Mds-Vo-name=local,o=grid"));

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
		    istring("explicity select or reject a specific cluster"),
		    istring("[-]name"),
		    clusters);

  std::list<std::string> indexurls;
  options.AddOption('i', "index",
		    istring("explicity select or reject an index server"),
		    istring("[-]name"),
		    indexurls);

  std::string joblist;
  options.AddOption('j', "joblist",
		    istring("file where the jobs will be stored"),
		    istring("filename"),
		    joblist);

  bool force = false;
  options.AddOption('f', "force",
                    istring("do not ask for verification"),
                    force);

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
    std::cout << Arc::IString("%s version %s", "arcsync", VERSION)
	      << std::endl;
    return 0;
  }

  //sanity check
  if (!force) {
    std::cout << Arc::IString("Synchronizing the local list of active jobs with the information in the MDS\n"
			      "can result in some inconsistencies. Very recently submitted jobs might not\n"
			      "yet be present in the MDS information, whereas jobs very recently scheduled\n"
			      "for deletion can still be present."
			      ) << std::endl;
    std::cout << Arc::IString("Are you sure you want to synchronize your local job list?") << " ["
	      << Arc::IString("y") << "/" << Arc::IString("n") << "] ";
    std::string response;
    std::cin >> response;
    /*
    if (response != Arc::IString("y")) {
      std::cout << Arc::IString("Cancelling synchronization request") << std::endl;
      return;
    }
    */
  }
  
  //Preparing the joblist file (create a new one if not existing)
  if (joblist.empty())
    joblist = usercfg.JobListFile();
  else {
    struct stat st;
    if (stat(joblist.c_str(), &st) != 0) {
      if (errno == ENOENT) {
	Arc::NS ns;
	Arc::Config(ns).SaveToFile(joblist);
	logger.msg(Arc::INFO, "Created empty ARC job list file: %s", joblist);
	stat(joblist.c_str(), &st);
      }
      else {
	logger.msg(Arc::ERROR, "Can not access ARC job list file: %s (%s)",
		   joblist, Arc::StrError());
	return 1;
      }
    }
    if (!S_ISREG(st.st_mode)) {
      logger.msg(Arc::ERROR, "ARC job list file is not a regular file: %s",
		 joblist);
      return 1;
    }
  }

  Arc::TargetGenerator targen(usercfg, clusters, indexurls);
  //  targen.GetAllJobs(0, 1);

  return 0;

}
