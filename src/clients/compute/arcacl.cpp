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
#include <arc/compute/JobControllerPlugin.h>
#include <arc/UserConfig.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
/*
#include <arc/StringConv.h>
#include <arc/compute/JobSupervisor.h>
*/

#include "utils.h"

int RUNMAIN(arcacl)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcacl");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_ACL,
                    istring("get|put [object ...]"),
                    istring("The arcacl command retrieves/sets permisions"
                            " (ACL) of data or computing object."));

  std::list<std::string> jobidentifiers = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcacl", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  if (jobidentifiers.empty()) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  std::string command = *(jobidentifiers.begin());
  jobidentifiers.erase(jobidentifiers.begin());
  if((command != "get") && (command != "put") && (command != "set")) {
    logger.msg(Arc::ERROR, "Unsupported command %s.", command);
    return 1;
  }

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:JobControllerPlugin");
    types.push_back("HED:DMC");
    showplugins("arcacl", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  for (std::list<std::string>::const_iterator it = opt.jobidinfiles.begin(); it != opt.jobidinfiles.end(); it++) {
    if (!Arc::Job::ReadJobIDsFromFile(*it, jobidentifiers)) {
      logger.msg(Arc::WARNING, "Cannot read specified jobid file: %s", *it);
    }
  }

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if ((!opt.joblist.empty() || !opt.status.empty()) && jobidentifiers.empty() && opt.clusters.empty())
    opt.all = true;

  if (jobidentifiers.empty() && opt.clusters.empty() && !opt.all) {
    logger.msg(Arc::ERROR, "No objects given");
    return 1;
  }

  std::list<std::string> selectedURLs = getSelectedURLsFromUserConfigAndCommandLine(usercfg, opt.clusters);
  std::list<std::string> rejectManagementURLs = getRejectManagementURLsFromUserConfigAndCommandLine(usercfg, opt.rejectmanagement);

  /*
  // If needed, read ACL from stdin
  std::string acl;
  if (command != "get") {
    if (acl.empty()) {
      logger.msg(Arc::ERROR, "ACL is empty");
      return 1;
    }
  }
  */

  // Currently there is no support for acl in job handling classes. Hence doing 
  // it directly. This also allows us to handle data objects.
  int retval = 0;

  // First try to find objects in jobs' file
  std::list<Arc::Job> jobs;
  if (!Arc::Job::ReadJobsFromFile(usercfg.JobListFile(), jobs, jobidentifiers, opt.all, selectedURLs, rejectManagementURLs)) {
    // If no jobs file still try and treat things like data objects
  }

  // Data part
  for (std::list<std::string>::const_iterator identifier = jobidentifiers.begin();
       identifier != jobidentifiers.end(); ++identifier) {
    logger.msg(Arc::VERBOSE, "Processing data object %s", *identifier);
    Arc::URL url(*identifier);
    if(!url) {
      logger.msg(Arc::ERROR, "Data object %s is not valid URL.", *identifier);
      retval = -1;
      continue;
    }
    if(url.Protocol() != "gsiftp") {
      logger.msg(Arc::ERROR, "Data object %s is not supported. Only GACL-enabled GridFTP servers are supported yet.", *identifier);
      retval = -1;
      continue;
    }
    std::string path = url.Path();
    std::string::size_type p = path.rfind('/');
    if(path.empty() || (p == (path.length()-1))) {
      path += ".gacl";
    } else {
      path.insert(p+1,".gacl-"); 
    }
    url.ChangePath(path);
    Arc::DataHandle obj(url, usercfg);
    if (!obj) {
      logger.msg(Arc::ERROR, "URL %s is not supported.", url.fullstr());
      retval = -1;
      continue;
    }
    Arc::DataMover mover;
    mover.secure(false);
    mover.passive(true);
    //mover.verbose(verbose);
    mover.checks(false);
    Arc::FileCache cache;
    Arc::URLMap map;
    Arc::DataStatus res;
    if(command=="get") {
      Arc::DataHandle tmp(Arc::URL("stdio:///stdout"), usercfg);
      if (!tmp) {
        logger.msg(Arc::ERROR, "Object for stdout handling failed.");
        retval = -1;
        continue;
      }
      res = mover.Transfer(*obj,*tmp,cache,map);
    } else {
      Arc::DataHandle tmp(Arc::URL("stdio:///stdin"), usercfg);
      if (!tmp) {
        logger.msg(Arc::ERROR, "Object for stdin handling failed.");
        retval = -1;
        continue;
      }
      res = mover.Transfer(*tmp,*obj,cache,map);
    }
    if(!res) {
      logger.msg(Arc::ERROR, "ACL transfer FAILED: %s", std::string(res));
      retval = -1;
      continue;
    }
  }

  // Jobs part - TODO

  return retval;
}
