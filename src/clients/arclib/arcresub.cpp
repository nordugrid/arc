// -*- indent-tabs-mode: nil -*-

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
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/client/Sandbox.h>
#include <arc/client/JobDescription.h>
#include <arc/client/JobController.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/UserConfig.h>
#include <arc/client/ACC.h>
#include <arc/client/Broker.h>
#include <arc/client/ACCLoader.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcresub");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("[job ...]\n"));

  bool all = false;
  options.AddOption('a', "all",
                    istring("all jobs"),
                    all);

  std::string joblist;
  options.AddOption('j', "joblist",
                    istring("file where the jobs will be stored"),
                    istring("filename"),
                    joblist);

  std::list<std::string> clusters;
  options.AddOption('c', "cluster",
                    istring("explicity select or reject a specific cluster"),
                    istring("[-]name"),
                    clusters);

  std::list<std::string> qlusters;
  options.AddOption('q', "qluster",
                    istring("explicity select or reject a specific cluster "
                            "for the new job"),
                    istring("[-]name"),
                    qlusters);

  std::list<std::string> indexurls;
  options.AddOption('i', "index",
                    istring("explicity select or reject an index server"),
                    istring("[-]name"),
                    indexurls);

  bool keep = false;
  options.AddOption('k', "keep",
                    istring("keep the files on the server (do not clean)"),
                    keep);

  bool same = false;
  options.AddOption('m', "same",
                    istring("resubmit to the same cluster"),
                    same);

  std::list<std::string> status;
  options.AddOption('s', "status",
                    istring("only select jobs whose status is statusstr"),
                    istring("statusstr"),
                    status);

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

  std::string broker;
  options.AddOption('b', "broker",
                    istring("select broker method (Random (default), QueueBalance, or custom)"),
                    istring("broker"), broker);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);


  std::list<std::string> jobs = options.Parse(argc, argv);

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
    std::cout << Arc::IString("%s version %s", "arcresub", VERSION)
              << std::endl;
    return 0;
  }

  if (jobs.empty() && !all) {
    std::cout << Arc::IString("No jobs given") << std::endl;
    logger.msg(Arc::ERROR, "No jobs given");
    return 1;
  }

  if (joblist.empty())
    joblist = usercfg.JobListFile();

  // Get jobs from sandbox
  Arc::JobSupervisor jobmaster(usercfg, jobs, clusters, joblist);
  std::list<Arc::JobController*> jobcont = jobmaster.GetJobControllers();
  if (jobcont.empty()) {
    logger.msg(Arc::ERROR, "No job controllers loaded");
    return 1;
  }

  std::list<Arc::Job> toberesubmitted;
  for (std::list<Arc::JobController*>::iterator it = jobcont.begin();
       it != jobcont.end(); it++) {
    std::list<Arc::Job> cont_jobs;
    cont_jobs = (*it)->GetJobDescriptions(status, true, timeout);
    toberesubmitted.insert(toberesubmitted.begin(), cont_jobs.begin(), cont_jobs.end());
  }
  if (toberesubmitted.empty()) {
    logger.msg(Arc::ERROR, "No jobs to resubmit");
    return 1;
  }

  // Preventing resubmitted jobs to be send to old clusters
  if (same)
    qlusters.clear();
  for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
       it != toberesubmitted.end(); it++)
    if (same) {
      qlusters.push_back(it->Flavour + ":" + it->Cluster.str());
      logger.msg(Arc::DEBUG, "Trying to resubmit job to %s", qlusters.front());
    }
    else {
      qlusters.remove(it->Flavour + ":" + it->Cluster.str());
      qlusters.push_back("-" + it->Flavour + ":" + it->Cluster.str());
      logger.msg(Arc::DEBUG, "Disregarding %s", it->Cluster.str());
    }
  qlusters.sort();
  qlusters.unique();

  // Resubmitting jobs
  Arc::TargetGenerator targen(usercfg, qlusters, indexurls);
  targen.GetTargets(0, 1);

  if (targen.FoundTargets().empty()) {
    std::cout << Arc::IString("Job submission aborted because no clusters returned any information") << std::endl;
    return 1;
  }
  Arc::NS ns;
  Arc::Config jobstorage(ns);

  //prepare loader
  if (broker.empty())
    broker = "RandomBroker";

  Arc::ACCConfig acccfg;
  Arc::Config cfg(ns);
  acccfg.MakeConfig(cfg);

  Arc::XMLNode Broker = cfg.NewChild("ArcClientComponent");
  std::string::size_type pos = broker.find(':');
  if (pos != std::string::npos) {
    Broker.NewAttribute("name") = broker.substr(0, pos);
    Broker.NewChild("Arguments") = broker.substr(pos + 1);
  }
  else
    Broker.NewAttribute("name") = broker;
  Broker.NewAttribute("id") = "broker";

  usercfg.ApplySecurity(Broker);

  Arc::ACCLoader loader(cfg);
  Arc::Broker *ChosenBroker = dynamic_cast<Arc::Broker*>(loader.getACC("broker"));
  logger.msg(Arc::INFO, "Broker %s loaded", broker);

  // Loop over jobs
  for (std::list<Arc::Job>::iterator it = toberesubmitted.begin();
       it != toberesubmitted.end(); it++) {

    Arc::JobDescription jobdesc;
    jobdesc.setSource(it->JobDescription);
    // Checking for previous OldJobIDs and adding this JobID
    for (std::list<Arc::URL>::iterator oldjob = it->OldJobIDs.begin();
         oldjob != it->OldJobIDs.end(); oldjob++)
      jobdesc.addOldJobID(*oldjob);
    jobdesc.addOldJobID(it->JobID);
    ChosenBroker->PreFilterTargets(targen, jobdesc);
    bool JobSubmitted = false;
    bool EndOfList = false;
    while (!JobSubmitted) {

      Arc::ExecutionTarget& target = ChosenBroker->GetBestTarget(EndOfList);
      if (EndOfList) {
        std::cout << Arc::IString("Job submission failed, no more possible targets") << std::endl;
        //Do not clean jobs that are not resubmitted
        jobs.remove(it->JobID.str());
        break;
      }

      Arc::Submitter *submitter = target.GetSubmitter(usercfg);

      Arc::NS ns;
      Arc::XMLNode info(ns, "Job");
      if (!submitter->Submit(jobdesc, info)) {
        std::cout << Arc::IString("Submission to %s failed, trying next target", target.url.str()) << std::endl;
        continue;
      }

      //need to get the jobinnerrepresentation in order to get the number of slots
      Arc::JobInnerRepresentation jir;
      jobdesc.getInnerRepresentation(jir);

      for (std::list<Arc::ExecutionTarget>::iterator target2 = targen.ModifyFoundTargets().begin(); target2 != targen.ModifyFoundTargets().end(); target2++)
        if (target.url == (*target2).url) {
          if ((*target2).FreeSlots >= abs(jir.Slots)) {   //The job will start directly
            (*target2).FreeSlots -= abs(jir.Slots);
            if ((*target2).UsedSlots != -1)
              (*target2).UsedSlots += abs(jir.Slots);
          }
          else                                           //The job will be queued
          if ((*target2).WaitingJobs != -1)
            (*target2).WaitingJobs += abs(jir.Slots);
        }
      Arc::XMLNode node;
      if (jobdesc.getXML(node))
        if ((bool)node["JobDescription"]["JobIdentification"]["JobName"])
          info.NewChild("Name") = (std::string)node["JobDescription"]["JobIdentification"]["JobName"];
      info.NewChild("Flavour") = target.GridFlavour;
      info.NewChild("Cluster") = target.Cluster.str();
      info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();
      if (!Arc::Sandbox::Add(jobdesc, info))
        logger.msg(Arc::ERROR, "Job not stored in sandbox");
      else
        logger.msg(Arc::VERBOSE, "Job description succesfully stored in sandbox");

      jobstorage.NewChild("Job").Replace(info);

      logger.msg(Arc::INFO, "Job resubmitted with new jobid: %s\n",
                 (std::string)info["JobID"]);
      JobSubmitted = true;
      break;
    } //end loop over all possible targets
  } //end loop over all job descriptions

  // Only kill and clean jobs that have been resubmitted
  clusters.clear();
  Arc::JobSupervisor killmaster(usercfg, jobs, clusters, joblist);
  std::list<Arc::JobController*> killcont = killmaster.GetJobControllers();
  if (killcont.empty()) {
    logger.msg(Arc::ERROR, "No job controllers loaded");
    return 1;
  }

  for (std::list<Arc::JobController*>::iterator it = killcont.begin();
       it != killcont.end(); it++)
    if (!(*it)->Kill(status, keep, timeout))
      if (!keep)
        if (!(*it)->Clean(status, true, timeout))
          logger.msg(Arc::WARNING, "Job could not be killed or cleaned");


  //now add info about all resubmitted jobs to the local xml file
  { //start of file lock
    Arc::FileLock lock(joblist);
    Arc::Config jobs;
    jobs.ReadFromFile(joblist);
    for (Arc::XMLNode j = jobstorage["Job"]; j; ++j) {
      jobs.NewChild(j);
      std::cout << Arc::IString("Job resubmitted with new jobid: %s\n",
                                (std::string)j["JobID"]) << std::endl;
    }
    jobs.SaveToFile(joblist);
  } //end of file lock

  /*
     if (toberesubmitted.size() > 1) {
     std::cout << std::endl << Arc::IString("Job submission summary:")
     << std::endl;
     std::cout << "-----------------------" << std::endl;
     std::cout << Arc::IString("%d of %d jobs were submitted",
     toberesubmitted.size() - notresubmitted.size(),
     toberesubmitted.size()) << std::endl;
     if (notresubmitted.size()) {
     std::cout << Arc::IString("The following %d were not submitted",
     notresubmitted.size()) << std::endl;
     }
     }*/
  return 0;
}
