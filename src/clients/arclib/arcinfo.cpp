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
#include <arc/client/TargetGenerator.h>
#include <arc/client/UserConfig.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcinfo");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]"),
                            istring("The arcinfo command is used for "
                                    "obtaining the status of clusters\n"
                                    "on the grid."),
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

  bool longlist = false;
  options.AddOption('l', "long",
                    istring("long format (more information)"),
                    longlist);

  int timeout = -1;
  options.AddOption('t', "timeout", istring("timeout in seconds (default " + Arc::tostring(Arc::UserConfig::DEFAULT_TIMEOUT) + ")"),
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

  std::list<std::string> jobs = options.Parse(argc, argv);

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  else if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (timeout > 0) {
    usercfg.SetTimeout(timeout);
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcinfo", VERSION)
              << std::endl;
    return 0;
  }

  // Proxy check
  if (!usercfg.CheckProxy())
    return 1;

  Arc::TargetGenerator targen(usercfg, clusters, indexurls);
  targen.GetTargets(0, 1);
  targen.PrintTargetInfo(longlist);
  return 0;
}
