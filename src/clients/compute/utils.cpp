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
      std::cout << Arc::IString("Types of execution services that %s is able to submit jobs to:", program) << std::endl;
    }
    else if (*itType == "HED:ServiceEndpointRetrieverPlugin") {
      std::cout << Arc::IString("Types of registry services that %s is able to collect information from:", program) << std::endl;
    }
    else if (*itType == "HED:TargetInformationRetrieverPlugin") {
      std::cout << Arc::IString("Types of local information services that %s is able to collect information from:", program) << std::endl;
    }
    else if (*itType == "HED:JobListRetriever") {
      std::cout << Arc::IString("Types of local information services that %s is able to collect job information from:", program) << std::endl;
    }
    else if (*itType == "HED:JobControllerPlugin") {
      std::cout << Arc::IString("Types of services that %s is able to manage jobs at:", program) << std::endl;
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

bool ClientOptions::isARC6TargetSelectionOptions(Arc::Logger& logger, bool allow_cluster) {
  bool arc6_target_options = false;
  do {
    if ( ! computing_elements.empty() ) { arc6_target_options = true; break; }
    if ( ! registries.empty() ) { arc6_target_options = true; break; }
    if ( ! requested_submission_endpoint_type.empty() ) { arc6_target_options = true; break; }
    if ( ! requested_info_endpoint_type.empty() ) arc6_target_options = true;
  } while (false);
  bool legacy_target_options = false;
  do {
    if ( ! clusters.empty() && !allow_cluster ) { legacy_target_options = true; break; }
    if ( ! indexurls.empty() ) { legacy_target_options = true; break; }
    if ( ! requestedSubmissionInterfaceName.empty() ) { legacy_target_options = true; break; }
    if ( ! infointerface.empty() ) { legacy_target_options = true; break; }
    if ( direct_submission ) legacy_target_options = true;
  } while (false);
  if ( legacy_target_options && arc6_target_options ) {
    logger.msg(Arc::ERROR, "It is impossible to mix ARC6 target selection options with legacy options. All legacy options will be ignored!");
  }
  return arc6_target_options;
}

bool ClientOptions::canonicalizeARC6InterfaceTypes(Arc::Logger& logger) {
  std::string s(requested_submission_endpoint_type);
  std::string i(requested_info_endpoint_type);
  // canonicalize submission endpoint
  if ( !s.empty() ) {
    if (s.find(".") == std::string::npos) {
      s = "org.nordugrid." + s;
    }
    // replace EMI-ES type
    if (s == "org.nordugrid.emies") {
      s = "org.ogf.glue.emies.activitycreation";
    // allow to use gridftp as gridftpjob
    } else if ( s == "org.nordugrid.gridftp" ) {
      s += "job";
    }
  }
  // canonicalize information endpoint
  if ( !i.empty() && Arc::lower(i) != "none" ) {
    if (i.find(".") == std::string::npos ) {
      i = "org.nordugrid." + i;
    } else if ( i == "ldap.nordugrid" ) {
      i = "org.nordugrid.ldapng";
    } else if ( i == "ldap.glue2" ) {
      i = "org.nordugrid.ldapglue2";
    }
    // replace EMI-ES type
    if (i == "org.nordugrid.emies") {
      i = "org.ogf.glue.emies.resourceinfo";
    // allow to use gridftp as gridftpjob
    } else if ( s == "org.nordugrid.gridftp" ) {
      s += "job";
    }
  }

  // nothing specified - any interface can be used
  if ( s.empty() && i.empty() ) return true;

  // define info based on submission (and verify submission type is supported)
  if ( !s.empty() ) {
    const std::string notify_template = "Automatically adding %s information endpoint type based on desired submission interface";
    if ( s == "org.ogf.glue.emies.activitycreation" ) {
      if ( i.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.ogf.glue.emies.resourceinfo");
        info_types.push_back("org.ogf.glue.emies.resourceinfo");
      }
    } else if ( s == "org.nordugrid.arcrest" ) {
      if ( i.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.arcrest");
        info_types.push_back("org.nordugrid.arcrest");
      }
    } else if ( s == "org.nordugrid.internal" ) {
      if ( i.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.internal");
        info_types.push_back("org.nordugrid.internal");
      }
    } else if ( s == "org.nordugrid.gridftpjob" ) {
      if ( i.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.ldapng");
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.ldapglue2");
        info_types.push_back("org.nordugrid.ldapng");
        info_types.push_back("org.nordugrid.ldapglue2");
      }
    } else {
      logger.msg(Arc::ERROR, "Unsupported submission endpoint type: %s", s);
      return false;
    }
    submit_types.push_back(s);
  }

  // define submission type based on info (and verify info type is supported)
  if ( !i.empty() ) {
    const std::string notify_template = "Automatically adding %s submission endpoint type based on desired information interface";
    if ( i == "org.ogf.glue.emies.resourceinfo" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.ogf.glue.emies.activitycreation");
        submit_types.push_back("org.ogf.glue.emies.activitycreation");
      }
    } else if ( i == "org.nordugrid.arcrest" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.arcrest");
        submit_types.push_back("org.nordugrid.arcrest");
      }
    } else if ( i == "org.nordugrid.internal" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.internal");
        submit_types.push_back("org.nordugrid.internal");
      }
    } else if ( i == "org.nordugrid.ldapng" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.gridftpjob");
        submit_types.push_back("org.nordugrid.gridftpjob");
      }
    } else if ( i == "org.nordugrid.ldapglue2" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.ogf.glue.emies.activitycreation");
        submit_types.push_back("org.ogf.glue.emies.activitycreation");
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.gridftpjob");
        submit_types.push_back("org.nordugrid.gridftpjob");
      }
    } else if ( Arc::lower(i) == "none" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, "Requested to skip resource discovery. Will try direct submission to %s and %s submission endpoint types", "org.ogf.glue.emies.activitycreation", "org.nordugrid.gridftpjob");
        submit_types.push_back("org.ogf.glue.emies.activitycreation");
        submit_types.push_back("org.nordugrid.gridftpjob");
      }
      return true;
    } else {
      logger.msg(Arc::ERROR, "Unsupported information endpoint type: %s", i);
      return false;
    }
    info_types.push_back(i);
  }
  
  return true;
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
    forceclean(false),
    show_stdout(true),
    show_stderr(false),
    show_joblog(false),
    show_json(false),
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

  DefineOptionsGroup("xaction", istring("Other actions"));
  DefineOptionsGroup("filtering", istring("Brokering and filtering"));
  DefineOptionsGroup("format", istring("Output format modifiers"));
  DefineOptionsGroup("tuning", istring("Behaviour tuning"));
  DefineOptionsGroup("arc6-target", istring("ARC6 submission endpoint selection"));
  DefineOptionsGroup("legacy-target", istring("Legacy options set for defining targets"));

  if ( c == CO_RESUB || c == CO_SUB || c == CO_TEST || c == CO_SYNC ) {
    GroupAddOption("arc6-target", 'C', "computing-element",
            istring("specify computing element hostname or a complete endpoint URL"),
            istring("ce"),
            computing_elements);

    GroupAddOption("arc6-target", 'Y', "registry",
            istring("registry service URL with optional specification of protocol"),
            istring("registry"),
            registries);
  }

  if ( c == CO_RESUB || c == CO_SUB || c == CO_TEST ) {
    GroupAddOption("arc6-target", 'T', "submission-endpoint-type",
            istring("require the specified endpoint type for job submission.\n"
                    "\tAllowed values are: arcrest, emies, gridftp or gridftpjob and internal."),
            istring("type"),
            requested_submission_endpoint_type);

    GroupAddOption("arc6-target", 'Q', "info-endpoint-type",
            istring("require information query using the specified information endpoint type.\n"
                    "\tSpecial value 'NONE' will disable all resource information queries and the following brokering.\n"
                    "\tAllowed values are: ldap.nordugrid, ldap.glue2, emies, arcrest and internal."),
            istring("type"),
            requested_info_endpoint_type);
  }

  if (c == CO_SUB || c == CO_TEST || c == CO_SYNC ) {
    GroupAddOption("legacy-target", 'c', "cluster",
              istring("select one or more computing elements: "
                      "name can be an alias for a single CE, a group of CEs or a URL"),
              istring("name"),
              clusters);
  } else {
    GroupAddOption("filtering", 'c', "cluster",
              istring("only select jobs that were submitted to this resource"),
              istring("name"),
              clusters);
  }
  
  if (!cIsJobMan && c != CO_SYNC) {
    GroupAddOption("legacy-target", 'I', "infointerface",
              istring("the computing element specified by URL at the command line "
                      "should be queried using this information interface.\n"
                      "\tAllowed values are: org.nordugrid.ldapng, org.nordugrid.ldapglue2 and org.ogf.glue.emies.resourceinfo"),
              istring("interfacename"),
              infointerface);
  }

  if (c == CO_RESUB || c == CO_MIGRATE) {
    GroupAddOption("legacy-target", 'q', "qluster",
              istring("selecting a computing element for the new jobs with a URL or an alias, "
                      "or selecting a group of computing elements with the name of the group"),
              istring("name"),
              qlusters);
  }

  if (c == CO_MIGRATE) {
    GroupAddOption("tuning", 'f', "force",
              istring("force migration, ignore kill failure"),
              forcemigration);
  }

  if (c == CO_GET || c == CO_KILL || c == CO_MIGRATE || c == CO_RESUB) {
    GroupAddOption("tuning", 'k', "keep",
              istring("keep the files on the server (do not clean)"),
              keep);
  }

  if (c == CO_SYNC) {
    GroupAddOption("tuning", 'f', "force",
              istring("do not ask for verification"),
              forcesync);

    GroupAddOption("tuning", 'T', "truncate",
              istring("truncate the joblist before synchronizing"),
              truncate);

    GroupAddOption("xaction", 0, "convert",
              istring("do not collect information, only convert jobs storage format"),
              convert);
  }

  if (c == CO_INFO || c == CO_STAT) {
    GroupAddOption("format", 'l', "long",
              istring("long format (more information)"),
              longlist);
  }

  if (c == CO_INFO) {
    GroupAddOption("xaction", 'L', "list-configured-services",
              istring("print a list of services configured in the client.conf"),
              list_configured_services);
  }

  if (c == CO_CAT) {
    GroupAddOption("xaction", 'o', "stdout",
              istring("show the stdout of the job (default)"),
              show_stdout);

    GroupAddOption("xaction", 'e', "stderr",
              istring("show the stderr of the job"),
              show_stderr);

    GroupAddOption("xaction", 'l', "joblog",
              istring("show the CE's error log of the job"),
              show_joblog);

    GroupAddOption("xaction", 'f', "file",
              istring("show the specified file from job's session directory"),
              istring("filepath"),
              show_file);
  }

  if (c == CO_GET) {
    GroupAddOption("tuning", 'D', "dir",
              istring("download directory (the job directory will"
                      " be created in this directory)"),
              istring("dirname"),
              downloaddir);

    GroupAddOption("tuning", 'J', "usejobname",
              istring("use the jobname instead of the short ID as"
                      " the job directory name"),
              usejobname);

    GroupAddOption("tuning", 'f', "force",
              istring("force download (overwrite existing job directory)"),
              forcedownload);
  }

  if (c == CO_STAT) {
    // Option 'long' takes precedence over this option (print-jobids).
    GroupAddOption("xaction", 'p', "print-jobids", istring("instead of the status only the IDs of "
              "the selected jobs will be printed"), printids);

    GroupAddOption("tuning", 'S', "sort",
              istring("sort jobs according to jobid, submissiontime or jobname"),
              istring("order"), sort);
    GroupAddOption("tuning", 'R', "rsort",
              istring("reverse sorting of jobs according to jobid, submissiontime or jobname"),
              istring("order"), rsort);

    GroupAddOption("tuning", 'u', "show-unavailable",
              istring("show jobs where status information is unavailable"),
              show_unavailable);

    GroupAddOption("format", 'J', "json",
              istring("show status information in JSON format"),
              show_json);
  }

  if (c == CO_RESUB) {
    GroupAddOption("filtering", 'm', "same",
              istring("resubmit to the same resource"),
              same);

    GroupAddOption("filtering", 'M', "not-same",
              istring("do not resubmit to the same resource"),
              notsame);
  }

  if (c == CO_CLEAN) {
    GroupAddOption("tuning", 'f', "force",
              istring("remove the job from the local list of jobs "
                      "even if the job is not found in the infosys"),
              forceclean);
  }

  if (!cIsJobMan) {
    GroupAddOption("legacy-target", 'g', "index",
              istring("select one or more registries: "
                      "name can be an alias for a single registry, a group of registries or a URL"),
              istring("name"),
              indexurls);
  }

  if (c == CO_TEST) {
    GroupAddOption("xaction", 'J', "job",
              istring("submit test job given by the number"),
              istring("int"),
              testjobid);
    GroupAddOption("xaction", 'r', "runtime",
              istring("test job runtime specified by the number"),
              istring("int"),
              runtime);
  }

  if (cIsJobMan || c == CO_RESUB) {
    GroupAddOption("filtering", 's', "status",
              istring("only select jobs whose status is statusstr"),
              istring("statusstr"),
              status);
  }

  if (cIsJobMan || c == CO_MIGRATE || c == CO_RESUB) {
    GroupAddOption("filtering", 'a', "all",
              istring("all jobs"),
              all);
  }

  if (c == CO_SUB) {
    GroupAddOption("tuning", 'e', "jobdescrstring",
              istring("jobdescription string describing the job to "
                      "be submitted"),
              istring("string"),
              jobdescriptionstrings);

    GroupAddOption("tuning", 'f', "jobdescrfile",
              istring("jobdescription file describing the job to "
                      "be submitted"),
              istring("string"),
              jobdescriptionfiles);
  }

  if (c == CO_MIGRATE || c == CO_RESUB || c == CO_SUB || c == CO_TEST) {
    GroupAddOption("filtering", 'b', "broker",
              istring("select broker method (list available brokers with --listplugins flag)"),
              istring("broker"), broker);

    GroupAddOption("tuning", 'o', "jobids-to-file",
              istring("the IDs of the submitted jobs will be appended to this file"),
              istring("filename"),
              jobidoutfile);
    
    GroupAddOption("legacy-target", 'S', "submissioninterface",
              istring("only use this interface for submitting.\n"
                      "\tAllowed values are: org.nordugrid.gridftpjob or org.nordugrid.gridftp, org.ogf.glue.emies.activitycreation and org.nordugrid.internal"),
              istring("InterfaceName"),
              requestedSubmissionInterfaceName);

  }
  
  if (c == CO_MIGRATE || c == CO_RESUB || c == CO_SUB || c == CO_TEST || c == CO_INFO) {
    GroupAddOption("filtering", 'R', "rejectdiscovery",
              istring("skip the service with the given URL during service discovery"),
              istring("URL"),
              rejectdiscovery);
  }
  

  if (cIsJobMan || c == CO_MIGRATE || c == CO_RESUB) {
    GroupAddOption("tuning", 'i', "jobids-from-file",
              istring("a file containing a list of jobIDs"),
              istring("filename"),
              jobidinfiles);

    GroupAddOption("filtering", 'r', "rejectmanagement",
              istring("skip jobs that are on a computing element with a given URL"),
              istring("URL"),
              rejectmanagement);    
  }

  if (c == CO_SUB || c == CO_TEST) {
    GroupAddOption("xaction", 'D', "dryrun", istring("submit jobs as dry run (no submission to batch system)"),
              dryrun);

    GroupAddOption("legacy-target", 0, "direct", istring("submit directly - no resource discovery or matchmaking"),
              direct_submission);

    GroupAddOption("xaction", 'x', "dumpdescription",
              istring("do not submit - dump job description "
                      "in the language accepted by the target"),
              dumpdescription);
  }
  
  if (c == CO_INFO) {
    GroupAddOption("legacy-target", 'S', "submissioninterface",
              istring("only get information about executon targets that support this job submission interface.\n"
                      "\tAllowed values are org.nordugrid.gridftpjob or org.nordugrid.gridftp, org.ogf.glue.emies.activitycreation and org.nordugrid.internal"),
              istring("InterfaceName"),
              requestedSubmissionInterfaceName);
  }

  if (c == CO_TEST) {
    GroupAddOption("xaction", 'E', "certificate", istring("prints info about installed user- and CA-certificates"), show_credentials);
  }

  if (c != CO_INFO) {
    GroupAddOption("tuning", 'j', "joblist",
              Arc::IString("the file storing information about active jobs (default %s)", Arc::UserConfig::JOBLISTFILE()).str(),
              istring("filename"),
              joblist);
  }

  /* --- Standard options below --- */

  AddOption('z', "conffile",
            istring("configuration file (default ~/.arc/client.conf)"),
            istring("filename"), conffile);

  AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
            istring("seconds"), timeout);

  GroupAddOption("xaction", 'P', "listplugins",
            istring("list the available plugins"),
            show_plugins);

  AddOption('d', "debug",
            istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
            istring("debuglevel"), debug);

  AddOption('v', "version", istring("print version information"),
            showversion);

}
