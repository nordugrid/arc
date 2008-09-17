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
			       const std::list<std::string>& jobs,
			       const std::list<std::string>& clusters,
			       const std::string& joblist)
    : loader(NULL) {

    URLListMap jobids;
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

    if (!jobs.empty()) {

      logger.msg(DEBUG, "Identifying needed job controllers according to "
		 "specified jobs");

      for (std::list<std::string>::const_iterator it = jobs.begin();
	   it != jobs.end(); it++) {

	XMLNodeList xmljobs =
	  jobstorage.XPathLookup("//Job[JobID='" + *it + "' or "
				 "Name='" + *it + "']", NS());

	if (xmljobs.empty()) {
	  logger.msg(WARNING, "Job not found in job list: %s", *it);
	  continue;
	}

	for (XMLNodeList::iterator xit = xmljobs.begin();
	     xit != xmljobs.end(); xit++) {

	  URL jobid = (std::string)(*xit)["JobID"];
	  std::string flavour = (std::string)(*xit)["Flavour"];

	  jobids[flavour].push_back(jobid);

	  if (std::find(controllers.begin(), controllers.end(),
			flavour) == controllers.end()) {
		logger.msg(DEBUG, "Need job controller for grid flavour %s",
			   flavour);
		controllers.push_back(flavour);
	  }
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

    if (jobs.empty() && clusterselect.empty()) {

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
	jobctrl->FillJobStore(jobids[jobctrl->Flavour()],
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
