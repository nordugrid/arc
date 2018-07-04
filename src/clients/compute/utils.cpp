// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <arc/ArcConfig.h>
#include <arc/IString.h>
#include <arc/credential/Credential.h>
#include <arc/loader/FinderLoader.h>
#include <arc/loader/Plugin.h>

#include "utils.h"

#ifndef WIN32

#include <unistd.h>
#include <termios.h>

ConsoleRecovery::ConsoleRecovery(void) {
  ti = new termios;
  if (tcgetattr(STDIN_FILENO, ti) == 0) return;
  delete ti;
  ti = NULL;
}

ConsoleRecovery::~ConsoleRecovery(void) {
  if(ti) tcsetattr(STDIN_FILENO, TCSANOW, ti);
  delete ti;
}

#else

ConsoleRecovery::ConsoleRecovery(void) { ti=NULL; }

ConsoleRecovery::~ConsoleRecovery(void) { }

#endif

std::list<std::string> getSelectedURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> computingelements) {
  std::list<Arc::Endpoint> endpoints = getServicesFromUserConfigAndCommandLine(usercfg, std::list<std::string>(), computingelements);
  std::list<std::string> serviceURLs;
  for (std::list<Arc::Endpoint>::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
    serviceURLs.push_back(it->URLString);
  }
  return serviceURLs;
}

std::list<std::string> getRejectDiscoveryURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectdiscovery) {
  std::list<std::string> rejectDiscoveryURLs = usercfg.RejectDiscoveryURLs();  
  rejectDiscoveryURLs.insert(rejectDiscoveryURLs.end(), rejectdiscovery.begin(), rejectdiscovery.end());
  return rejectDiscoveryURLs;
}

std::list<std::string> getRejectManagementURLsFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> rejectmanagement) {
  std::list<std::string> rejectManagementURLs = usercfg.RejectManagementURLs();  
  rejectManagementURLs.insert(rejectManagementURLs.end(), rejectmanagement.begin(), rejectmanagement.end());
  return rejectManagementURLs;
}


std::list<Arc::Endpoint> getServicesFromUserConfigAndCommandLine(Arc::UserConfig usercfg, std::list<std::string> registries, std::list<std::string> computingelements, std::string requestedSubmissionInterfaceName, std::string infointerface) {
  std::list<Arc::Endpoint> services;
  if (computingelements.empty() && registries.empty()) {
    std::list<Arc::ConfigEndpoint> endpoints = usercfg.GetDefaultServices();
    for (std::list<Arc::ConfigEndpoint>::const_iterator its = endpoints.begin(); its != endpoints.end(); ++its) {
      services.push_back(*its);
    }
  } else {
    for (std::list<std::string>::const_iterator it = computingelements.begin(); it != computingelements.end(); ++it) {
      // check if the string is a group or alias
      std::list<Arc::ConfigEndpoint> newServices = usercfg.GetServices(*it, Arc::ConfigEndpoint::COMPUTINGINFO);
      if (newServices.empty()) {
          // if it was not an alias or a group, then it should be the URL
          Arc::Endpoint service(*it);
          service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::COMPUTINGINFO));
          if (!infointerface.empty()) {
            service.InterfaceName = infointerface;            
          }
          service.RequestedSubmissionInterfaceName = requestedSubmissionInterfaceName;
          services.push_back(service);
      } else {
        // if it was a group (or an alias), add all the services
        for (std::list<Arc::ConfigEndpoint>::iterator its = newServices.begin(); its != newServices.end(); ++its) {
          if (!requestedSubmissionInterfaceName.empty()) {
            // if there was a submission interface requested, this overrides the one from the config
            its->RequestedSubmissionInterfaceName = requestedSubmissionInterfaceName;    
          }
          services.push_back(*its);
        }
      }
    }
    for (std::list<std::string>::const_iterator it = registries.begin(); it != registries.end(); ++it) {
      // check if the string is a name of a group
      std::list<Arc::ConfigEndpoint> newServices = usercfg.GetServices(*it, Arc::ConfigEndpoint::REGISTRY);
      if (newServices.empty()) {
          // if it was not an alias or a group, then it should be the URL
          Arc::Endpoint service(*it);
          service.Capability.insert(Arc::Endpoint::GetStringForCapability(Arc::Endpoint::REGISTRY));
          services.push_back(service);
      } else {
        // if it was a group (or an alias), add all the services
        services.insert(services.end(), newServices.begin(), newServices.end());
      }
    }
  }
  return services;
}


void showplugins(const std::string& program, const std::list<std::string>& types, Arc::Logger& logger, const std::string& chosenBroker) {

  for (std::list<std::string>::const_iterator itType = types.begin();
       itType != types.end(); ++itType) {
    if (*itType == "HED:SubmitterPlugin") {
      std::cout << Arc::IString("Types of execution services %s is able to submit jobs to:", program) << std::endl;
    }
    else if (*itType == "HED:ServiceEndpointRetrieverPlugin") {
      std::cout << Arc::IString("Types of registry services which %s is able collect information from:", program) << std::endl;
    }
    else if (*itType == "HED:TargetInformationRetrieverPlugin") {
      std::cout << Arc::IString("Types of local information services which %s is able collect information from:", program) << std::endl;
    }
    else if (*itType == "HED:JobListRetriever") {
      std::cout << Arc::IString("Types of local information services which %s is able collect job information from:", program) << std::endl;
    }
    else if (*itType == "HED:JobControllerPlugin") {
      std::cout << Arc::IString("Types of services %s is able to manage jobs at:", program) << std::endl;
    }
    else if (*itType == "HED:JobDescriptionParserPlugin") {
      std::cout << Arc::IString("Job description languages supported by %s:", program) << std::endl;
    }
    else if (*itType == "HED:BrokerPlugin") {
      std::cout << Arc::IString("Brokers available to %s:", program) << std::endl;
    }

    std::list<Arc::ModuleDesc> modules;
    Arc::PluginsFactory pf(Arc::BaseConfig().MakeConfig(Arc::Config()).Parent());

    bool isDefaultBrokerLocated = false;
    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind(*itType, modules);
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); ++itMod) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); ++itPlug) {
        std::cout << "  " << itPlug->name;
        if (*itType == "HED:BrokerPlugin" && itPlug->name == chosenBroker) {
          std::cout << " (default)";
          isDefaultBrokerLocated = true;
        }
        std::cout << " - " << itPlug->description << std::endl;
      }
    }

    if (*itType == "HED:BrokerPlugin" && !isDefaultBrokerLocated) {
      logger.msg(Arc::WARNING, "Default broker (%s) is not available. When using %s a broker should be specified explicitly (-b option).", chosenBroker, program);
    }
  }
}

bool checkproxy(const Arc::UserConfig& uc)
{
  if (!uc.ProxyPath().empty() ) {
    Arc::Credential holder(uc.ProxyPath(), "", "", "");
    if (holder.GetEndTime() < Arc::Time()){
      std::cout << Arc::IString("Proxy expired. Job submission aborted. Please run 'arcproxy'!") << std::endl;
      return false;
    }
  }
  else {
    std::cout << Arc::IString("Cannot find any proxy. This application currently cannot run without a proxy.\n"
                              "  If you have the proxy file in a non-default location,\n"
                              "  please make sure the path is specified in the client configuration file.\n"
                              "  If you don't have a proxy yet, please run 'arcproxy'!") << std::endl;
    return false;
  }

  return true;
}

void splitendpoints(std::list<std::string>& selected, std::list<std::string>& rejected)
{
  // Removes slashes from end of endpoint strings, and put strings with leading '-' into rejected list.
  for (std::list<std::string>::iterator it = selected.begin();
       it != selected.end();) {
    if ((*it)[it->length()-1] == '/') {
      it->erase(it->length()-1);
      continue;
    }
    if (it->empty()) {
      it = selected.erase(it);
      continue;
    }

    if ((*it)[0] == '-') {
      rejected.push_back(it->substr(1));
      it = selected.erase(it);
    }
    else {
      ++it;
    }
  }
}

Arc::JobInformationStorage* createJobInformationStorage(const Arc::UserConfig& uc) {
  Arc::JobInformationStorage* jis = NULL;
  if (Glib::file_test(uc.JobListFile(), Glib::FILE_TEST_EXISTS)) {
    for (int i = 0; Arc::JobInformationStorage::AVAILABLE_TYPES[i].name != NULL; ++i) {
      jis = (Arc::JobInformationStorage::AVAILABLE_TYPES[i].instance)(uc.JobListFile());
      if (jis && jis->IsValid()) {
        return jis;
      }
      delete jis;
    }
    return NULL;
  }
  
  for (int i = 0; Arc::JobInformationStorage::AVAILABLE_TYPES[i].name != NULL; ++i) {
    if (uc.JobListType() == Arc::JobInformationStorage::AVAILABLE_TYPES[i].name) {
      jis = (Arc::JobInformationStorage::AVAILABLE_TYPES[i].instance)(uc.JobListFile());
      if (jis && jis->IsValid()) {
        return jis;
      }
      delete jis;
      return NULL;
    }
  }

  if (Arc::JobInformationStorage::AVAILABLE_TYPES[0].instance != NULL) {
    jis = (Arc::JobInformationStorage::AVAILABLE_TYPES[0].instance)(uc.JobListFile());
    if (jis && jis->IsValid()) {
      return jis;
    }
    delete jis;
  }
  
  return NULL;
}


ClientOptions::ClientOptions(Client_t c,
                             const std::string& arguments,
                             const std::string& summary,
                             const std::string& description) :
    Arc::OptionParser(arguments, summary, description),
    dryrun(false),
    dumpdescription(false),
    show_credentials(false),
    show_plugins(false),
    showversion(false),
    all(false),
    forcemigration(false),
    keep(false),
    forcesync(false),
    truncate(false),
    convert(false),
    longlist(false),
    printids(false),
    same(false),
    notsame(false),
    show_stdout(true),
    show_stderr(false),
    show_joblog(false),
    usejobname(false),
    forcedownload(false),
    list_configured_services(false),
    direct_submission(false),
    show_unavailable(false),
    testjobid(-1),
    runtime(5),
    timeout(-1)
{
  bool cIsJobMan = (c == CO_CAT || c == CO_CLEAN || c == CO_GET || c == CO_KILL || c == CO_RENEW || c == CO_RESUME || c == CO_STAT || c == CO_ACL);

  AddOption('c', "cluster",
            istring("select one or more computing elements: "
                    "name can be an alias for a single CE, a group of CEs or a URL"),
            istring("name"),
            clusters);
  
  if (!cIsJobMan && c != CO_SYNC) {
    AddOption('I', "infointerface",
              istring("the computing element specified by URL at the command line "
                      "should be queried using this information interface "
                      "(possible options: org.nordugrid.ldapng, org.nordugrid.ldapglue2, "
                      "org.nordugrid.wsrfglue2, org.ogf.glue.emies.resourceinfo)"),
              istring("interfacename"),
              infointerface);
  }

  if (c == CO_RESUB || c == CO_MIGRATE) {
    AddOption('q', "qluster",
              istring("selecting a computing element for the new jobs with a URL or an alias, "
                      "or selecting a group of computing elements with the name of the group"),
              istring("name"),
              qlusters);
  }

  if (c == CO_MIGRATE) {
    AddOption('f', "force",
              istring("force migration, ignore kill failure"),
              forcemigration);
  }

  if (c == CO_GET || c == CO_KILL || c == CO_MIGRATE || c == CO_RESUB) {
    AddOption('k', "keep",
              istring("keep the files on the server (do not clean)"),
              keep);
  }

  if (c == CO_SYNC) {
    AddOption('f', "force",
              istring("do not ask for verification"),
              forcesync);

    AddOption('T', "truncate",
              istring("truncate the joblist before synchronizing"),
              truncate);

    AddOption('C', "convert",
              istring("do not collect information, only convert jobs storage format"),
              convert);
  }

  if (c == CO_INFO || c == CO_STAT) {
    AddOption('l', "long",
              istring("long format (more information)"),
              longlist);
  }

  if (c == CO_INFO) {
    AddOption('L', "list-configured-services",
              istring("print a list of services configured in the client.conf"),
              list_configured_services);
  }

  if (c == CO_CAT) {
    AddOption('o', "stdout",
              istring("show the stdout of the job (default)"),
              show_stdout);

    AddOption('e', "stderr",
              istring("show the stderr of the job"),
              show_stderr);

    AddOption('l', "joblog",
              istring("show the CE's error log of the job"),
              show_joblog);
  }

  if (c == CO_GET) {
    AddOption('D', "dir",
              istring("download directory (the job directory will"
                      " be created in this directory)"),
              istring("dirname"),
              downloaddir);

    AddOption('J', "usejobname",
              istring("use the jobname instead of the short ID as"
                      " the job directory name"),
              usejobname);

    AddOption('f', "force",
              istring("force download (overwrite existing job directory)"),
              forcedownload);
  }

  if (c == CO_STAT) {
    // Option 'long' takes precedence over this option (print-jobids).
    AddOption('p', "print-jobids", istring("instead of the status only the IDs of "
              "the selected jobs will be printed"), printids);

    AddOption('S', "sort",
              istring("sort jobs according to jobid, submissiontime or jobname"),
              istring("order"), sort);
    AddOption('R', "rsort",
              istring("reverse sorting of jobs according to jobid, submissiontime or jobname"),
              istring("order"), rsort);

    AddOption('u', "show-unavailable",
              istring("show jobs where status information is unavailable"),
              show_unavailable);
  }

  if (c == CO_RESUB) {
    AddOption('m', "same",
              istring("resubmit to the same resource"),
              same);

    AddOption('M', "not-same",
              istring("do not resubmit to the same resource"),
              notsame);
  }

  if (c == CO_CLEAN) {
    AddOption('f', "force",
              istring("remove the job from the local list of jobs "
                      "even if the job is not found in the infosys"),
              forceclean);
  }

  if (!cIsJobMan) {
    AddOption('g', "index",
              istring("select one or more registries: "
                      "name can be an alias for a single registry, a group of registries or a URL"),
              istring("name"),
              indexurls);
  }

  if (c == CO_TEST) {
    AddOption('J', "job",
              istring("submit test job given by the number"),
              istring("int"),
              testjobid);
    AddOption('r', "runtime",
              istring("test job runtime specified by the number"),
              istring("int"),
              runtime);
  }

  if (cIsJobMan || c == CO_RESUB) {
    AddOption('s', "status",
              istring("only select jobs whose status is statusstr"),
              istring("statusstr"),
              status);
  }

  if (cIsJobMan || c == CO_MIGRATE || c == CO_RESUB) {
    AddOption('a', "all",
              istring("all jobs"),
              all);
  }

  if (c == CO_SUB) {
    AddOption('e', "jobdescrstring",
              istring("jobdescription string describing the job to "
                      "be submitted"),
              istring("string"),
              jobdescriptionstrings);

    AddOption('f', "jobdescrfile",
              istring("jobdescription file describing the job to "
                      "be submitted"),
              istring("string"),
              jobdescriptionfiles);
  }

  if (c == CO_MIGRATE || c == CO_RESUB || c == CO_SUB || c == CO_TEST) {
    AddOption('b', "broker",
              istring("select broker method (list available brokers with --listplugins flag)"),
              istring("broker"), broker);

    AddOption('o', "jobids-to-file",
              istring("the IDs of the submitted jobs will be appended to this file"),
              istring("filename"),
              jobidoutfile);
    
    AddOption('S', "submissioninterface",
              istring("only use this interface for submitting "
                      "(e.g. org.nordugrid.gridftpjob, org.ogf.glue.emies.activitycreation, org.ogf.bes, org.nordugrid.internal)"),
              istring("InterfaceName"),
              requestedSubmissionInterfaceName);

  }
  
  if (c == CO_MIGRATE || c == CO_RESUB || c == CO_SUB || c == CO_TEST || c == CO_INFO) {
    AddOption('R', "rejectdiscovery",
              istring("skip the service with the given URL during service discovery"),
              istring("URL"),
              rejectdiscovery);
  }
  

  if (cIsJobMan || c == CO_MIGRATE || c == CO_RESUB) {
    AddOption('i', "jobids-from-file",
              istring("a file containing a list of jobIDs"),
              istring("filename"),
              jobidinfiles);

    AddOption('r', "rejectmanagement",
              istring("skip jobs which are on a computing element with a given URL"),
              istring("URL"),
              rejectmanagement);    
  }

  if (c == CO_SUB || c == CO_TEST) {
    AddOption('D', "dryrun", istring("submit jobs as dry run (no submission to batch system)"),
              dryrun);

    AddOption(0, "direct", istring("submit directly - no resource discovery or matchmaking"),
              direct_submission);

    AddOption('x', "dumpdescription",
              istring("do not submit - dump job description "
                      "in the language accepted by the target"),
              dumpdescription);
  }
  
  if (c == CO_INFO) {
    AddOption('S', "submissioninterface",
              istring("only get information about executon targets which support this job submission interface "
                      "(e.g. org.nordugrid.gridftpjob, org.ogf.glue.emies.activitycreation, org.ogf.bes, org.nordugrid.internal)"),
              istring("InterfaceName"),
              requestedSubmissionInterfaceName);
  }

  if (c == CO_TEST) {
    AddOption('E', "certificate", istring("prints info about installed user- and CA-certificates"), show_credentials);
  }

  if (c != CO_INFO) {
    AddOption('j', "joblist",
              Arc::IString("the file storing information about active jobs (default %s)", Arc::UserConfig::JOBLISTFILE).str(),
              istring("filename"),
              joblist);
  }

  /* --- Standard options below --- */

  AddOption('z', "conffile",
            istring("configuration file (default ~/.arc/client.conf)"),
            istring("filename"), conffile);

  AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
            istring("seconds"), timeout);

  AddOption('P', "listplugins",
            istring("list the available plugins"),
            show_plugins);

  AddOption('d', "debug",
            istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
            istring("debuglevel"), debug);

  AddOption('v', "version", istring("print version information"),
            showversion);

}
