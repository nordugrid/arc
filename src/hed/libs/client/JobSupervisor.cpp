#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <iostream>

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>

namespace Arc {

  Logger JobSupervisor::logger(Logger::getRootLogger(), "JobSupervisor");

  JobSupervisor::JobSupervisor(const UserConfig& usercfg,
			       const std::list<URL>& jobids,
			       const std::list<std::string>& clusters,
			       const std::string& joblist)
    : loader(NULL) {

    URLListMap clusterselect;
    URLListMap clusterreject;

    if (!usercfg.ResolveAlias(clusters, clusterselect, clusterreject)) {
      logger.msg(Arc::ERROR, "Failed resolving aliases");
      return;
    }

    Config jobstorage;
    if (!joblist.empty()) {
      FileLock lock(joblist);
      jobstorage.ReadFromFile(joblist);
    }

    std::list<std::string> controllers;

    if (!jobids.empty()) {

      logger.msg(DEBUG, "Identifying needed job controllers according to "
		 "specified jobids");

      for (std::list<URL>::const_iterator it = jobids.begin();
	   it != jobids.end(); it++) {

	XMLNodeList xmljobs =
	  jobstorage.XPathLookup("//Job[JobID='" + it->str() + "']", NS());

	if (xmljobs.empty()) {
	  logger.msg(DEBUG, "Job not found in job list: %s", it->str());
	  continue;
	}

	XMLNode& xmljob = *xmljobs.begin();

	if (std::find(controllers.begin(), controllers.end(),
		      (std::string)xmljob["Flavour"]) == controllers.end()) {
	  std::string flavour = (std::string)xmljob["Flavour"];
	  logger.msg(DEBUG, "Need job controller for grid flavour %s",
		     flavour);
	  controllers.push_back(flavour);
	}
      }
    }

    if (!clusterselect.empty()) {

      logger.msg(DEBUG, "Identifying needed job controllers according to "
		 "specified clusters");

      for (URLListMap::iterator it = clusterselect.begin();
	   it != clusterselect.end(); it++)
	if (std::find(controllers.begin(), controllers.end(),
		      it->first) == controllers.end()) {
	  logger.msg(DEBUG, "Need job controller for grid flavour %s",
		     it->first);
	  controllers.push_back(it->first);
	}
    }

    if (jobids.empty() && clusterselect.empty()) {

      logger.msg(DEBUG, "Identifying needed job controllers according to "
		 "all jobs present in job list");

      XMLNodeList xmljobs = jobstorage.XPathLookup("/ArcConfig/Job", NS());

      for (XMLNodeList::iterator it = xmljobs.begin();
	   it != xmljobs.end(); it++)
	if (std::find(controllers.begin(), controllers.end(),
		      (std::string)(*it)["Flavour"]) == controllers.end()) {
	  std::string flavour = (*it)["Flavour"];
	  logger.msg(DEBUG, "Need job controller for grid flavour %s",
		     flavour);
	  controllers.push_back(flavour);
	}
    }

    ACCConfig acccfg;
    NS ns;
    Config cfg(ns);
    acccfg.MakeConfig(cfg);
    int ctrlnum = 0;

    for (std::list<std::string>::iterator it = controllers.begin();
	 it != controllers.end(); it++) {

      XMLNode jobctrl = cfg.NewChild("ArcClientComponent");
      jobctrl.NewAttribute("name") = "JobController" + (*it);
      jobctrl.NewAttribute("id") = "controller" + tostring(ctrlnum);
      usercfg.ApplySecurity(jobctrl);
      jobctrl.NewChild("JobList") = joblist;
      ctrlnum++;
    }

    loader = new Loader(&cfg);

    for (int i = 0; i < ctrlnum; i++) {
      JobController *jobctrl =
	dynamic_cast<JobController*>(loader->getACC("controller" +
						    tostring(i)));
      if (jobctrl) {
	jobctrl->FillJobStore(jobids,
			      clusterselect[jobctrl->Flavour()],
			      clusterreject[jobctrl->Flavour()]);
	jobcontrollers.push_back(jobctrl);
      }
    }
  }

  JobSupervisor::~JobSupervisor() {
    if (loader)
      delete loader;
  }

} // namespace Arc
