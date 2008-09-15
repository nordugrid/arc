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
#include <arc/client/UserConfig.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arccat");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]"),
			    istring("The arccat command performs the cat "
				    "command on the stdout, stderr or grid\n"
				    "manager's error log of the job."),
			    istring("Argument to -c has the format "
				    "Flavour:URL e.g.\n"
				    "ARC0:ldap://grid.tsl.uu.se:2135/"
				    "nordugrid-cluster-name=grid.tsl.uu.se,"
				    "Mds-Vo-name=local,o=grid"));

  bool all = false;
  options.AddOption('a', "all",
		    istring("all jobs"),
		    all);

  std::string joblist;
  options.AddOption('j', "joblist",
		    istring("file containing a list of jobs"),
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

  bool show_stdout = true;
  options.AddOption('o', "stdout",
		    istring("show the stdout of the job (default)"),
		    show_stdout);

  bool show_stderr = false;
  options.AddOption('e', "stderr",
		    istring("show the stderr of the job"),
		    show_stderr);

  bool show_gmlog = false;
  options.AddOption('l', "gmlog",
		    istring("show the grid manager's error log of the job"),
		    show_gmlog); 

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
    std::cout << Arc::IString("%s version %s", "arccat", VERSION)
	      << std::endl;
    return 0;
  }

  std::list<Arc::URL> jobids;
  jobids.insert(jobids.end(), params.begin(), params.end());

  if (jobids.empty() && joblist.empty() && !all) {
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

  std::string whichfile;
  if (show_gmlog)
    whichfile = "gmlog";
  else if (show_stderr)
    whichfile = "stderr";
  else
    whichfile = "stdout";

  int retval = 0;
  for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
       it != jobcont.end(); it++)
    if (!(*it)->Cat(jobids, status, whichfile, timeout))
      retval = 1;

  return retval;
}
