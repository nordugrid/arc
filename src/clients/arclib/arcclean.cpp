// -*- indent-tabs-mode: nil -*-

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
#include <arc/StringConv.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/UserConfig.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcclean");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]"),
                            istring("The arcclean command removes a job "
                                    "from the remote cluster."),
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

  bool force = false;
  options.AddOption('f', "force",
                    istring("remove the job from the local list of jobs "
                            "even if the job is not found in the infosys"),
                    force);

  int timeout = -1;
  options.AddOption('t', "timeout", istring("timeout in seconds (default " + Arc::tostring(Arc::UserConfig::DEFAULT_TIMEOUT) + ")"),
                    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.conf)"),
                    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> jobs = options.Parse(argc, argv);

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile, joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (timeout > 0)
    usercfg.Timeout(timeout);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcclean", VERSION)
              << std::endl;
    return 0;
  }

  if (!joblist.empty() && jobs.empty() && clusters.empty())
    all = true;

  if (jobs.empty() && clusters.empty() && !all) {
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  if (!jobs.empty() || all)
    usercfg.ClearSelectedServices();

  if (!clusters.empty()) {
    usercfg.ClearSelectedServices();
    usercfg.AddServices(clusters, Arc::COMPUTING);
  }

  Arc::JobSupervisor jobmaster(usercfg, jobs);
  std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();

  // If the user specified a joblist on the command line joblist equals
  // usercfg.JobListFile(). If not use the default, ie. usercfg.JobListFile().
  if (jobcont.empty()) {
    logger.msg(Arc::ERROR, "No job controllers loaded");
    return 1;
  }

  int retval = 0;
  for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
       it != jobcont.end(); it++)
    if (!(*it)->Clean(status, force))
      retval = 1;

  return retval;
}
