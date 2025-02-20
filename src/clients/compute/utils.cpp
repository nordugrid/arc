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
    Arc::Credential holder(uc.ProxyPath(), "", "", "", false);
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

bool checktoken(const Arc::UserConfig& uc) {
  if(uc.OToken().empty()) {
    std::cout << Arc::IString("Cannot find any token. Please run 'oidc-token' or use similar\n"
		              "  utility to obtain authentication token!") << std::endl;
    return false;
  }
  return true;
}

static bool urlisinsecure(Arc::URL const & url) {
  std::string protocol = url.Protocol();
  return protocol.empty() || (protocol == "http") || (protocol == "ftp") || (protocol == "ldap");
}

bool jobneedsproxy(const Arc::JobDescription& job) {
  // Check if X.509 credentials are needed for data staging
  std::list<Arc::InputFileType> inputFiles = job.DataStaging.InputFiles;
  for(std::list<Arc::InputFileType>::iterator fileIt = inputFiles.begin(); fileIt != inputFiles.end(); ++fileIt) {
    for(std::list<Arc::SourceType>::iterator sourceIt = fileIt->Sources.begin(); sourceIt != fileIt->Sources.end(); ++sourceIt) {
      if(!urlisinsecure(*sourceIt)) {
        return true;
      }
    }
  }
  std::list<Arc::OutputFileType> outputFiles = job.DataStaging.OutputFiles;
  for(std::list<Arc::OutputFileType>::iterator fileIt = outputFiles.begin(); fileIt != outputFiles.end(); ++fileIt) {
    for(std::list<Arc::TargetType>::iterator targetIt = fileIt->Targets.begin(); targetIt != fileIt->Targets.end(); ++targetIt) {
      if(!urlisinsecure(*targetIt)) {
        return true;
      }
    }
  }
  return false;
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

bool ClientOptions::canonicalizeARCInterfaceTypes(Arc::Logger& logger) {
  std::string s(requested_submission_endpoint_type);
  std::string i(requested_info_endpoint_type);
  // canonicalize submission endpoint
  if ( !s.empty() ) {
    if (s.find(".") == std::string::npos) {
      s = "org.nordugrid." + s;
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
  }

  // nothing specified - any interface can be used
  if ( s.empty() && i.empty() ) return true;

  // define info based on submission (and verify submission type is supported)
  if ( !s.empty() ) {
    const std::string notify_template = "Automatically adding %s information endpoint type based on desired submission interface";
    if ( s == "org.nordugrid.arcrest" ) {
      if ( i.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.arcrest");
        info_types.push_back("org.nordugrid.arcrest");
      }
    } else if ( s == "org.nordugrid.internal" ) {
      if ( i.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.internal");
        info_types.push_back("org.nordugrid.internal");
      }
    } else {
      logger.msg(Arc::ERROR, "Unsupported submission endpoint type: %s", s);
      return false;
    }
    submit_types.push_back(s);
  }

  // define submission type based on info (and verify info type is supported)
  if ( !i.empty() ) {
    const std::string notify_template = "Add arcrest submission endpoint type.";
    if ( s.empty() ) {
      logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.arcrest");
      submit_types.push_back("org.nordugrid.arcrest");
    } else if ( i == "org.nordugrid.internal" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, notify_template, "org.nordugrid.internal");
        submit_types.push_back("org.nordugrid.internal");
      }
    } else if ( Arc::lower(i) == "none" ) {
      if ( s.empty() ) {
        logger.msg(Arc::VERBOSE, "Requested to skip resource discovery. Will try direct submission to arcrest endpoint type.");
        submit_types.push_back("org.nordugrid.arcrest");
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
    keep(false),
    forcesync(false),
    truncate(false),
    convert(false),
    longlist(false),
    printids(false),
    forceclean(false),
    show_stdout(true),
    show_stderr(false),
    show_joblog(false),
    show_json(false),
    usejobname(false),
    forcedownload(false),
    direct_submission(false),
    show_unavailable(false),
    no_delegation(false),
    x509_delegation(false),
    token_delegation(false),
    no_authentication(false),
    x509_authentication(false),
    token_authentication(false),
    force_system_ca(false),
    force_grid_ca(false),
    allow_insecure_connection(false),
    testjobid(-1),
    runtime(5),
    timeout(-1),
    instances_min(1),
    instances_max(1)
{
  bool cIsJobMan = (c == CO_CAT || c == CO_CLEAN || c == CO_GET || c == CO_KILL || c == CO_RENEW || c == CO_RESUME || c == CO_STAT || c == CO_ACL);

  DefineOptionsGroup("xaction", istring("Other actions"));
  DefineOptionsGroup("filtering", istring("Brokering and filtering"));
  DefineOptionsGroup("format", istring("Output format modifiers"));
  DefineOptionsGroup("tuning", istring("Behaviour tuning"));
  DefineOptionsGroup("arc-target", istring("Target endpoint selection"));

  if ( c == CO_SUB || c == CO_TEST || c == CO_SYNC || c == CO_INFO ) {
    GroupAddOption("arc-target", 'C', "computing-element",
            istring("computing element hostname or a complete endpoint URL"),
            istring("ce"),
            computing_elements);

    GroupAddOption("arc-target", 'Y', "registry",
            istring("registry service URL with optional specification of protocol"),
            istring("registry"),
            registries);
  } else {
    GroupAddOption("filtering", 'C', "computing-element",
            istring("only select jobs that were submitted to this computing element"),
            istring("ce"),
            computing_elements);
  }

  if ( c == CO_SUB || c == CO_TEST ) {
    GroupAddOption("arc-target", 'T', "submission-endpoint-type",
            istring("require the specified endpoint type for job submission.\n"
                    "\tAllowed values are: arcrest and internal."),
            istring("type"),
            requested_submission_endpoint_type);
  }

  if (c == CO_SUB || c == CO_TEST || c == CO_INFO) {
    GroupAddOption("filtering", 'R', "rejectdiscovery",
            istring("skip the service with the given URL during service discovery"),
            istring("URL"),
            rejectdiscovery);

    GroupAddOption("arc-target", 'Q', "info-endpoint-type",
          istring("require information query using the specified information endpoint type.\n"
                  "\tSpecial value 'NONE' will disable all resource information queries and the following brokering.\n"
                  "\tAllowed values are: ldap.nordugrid, ldap.glue2, arcrest and internal."),
          istring("type"),
          requested_info_endpoint_type);
  }

  if (c == CO_INFO) {
    GroupAddOption("arc-target", 'T', "submission-endpoint-type",
          istring("only get information about executon targets that support this job submission endpoint type.\n"
                  "\tAllowed values are: arcrest and internal."),
          istring("type"),
          requested_submission_endpoint_type);
  }

  if (c == CO_GET || c == CO_KILL ) {
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

  if (c == CO_CLEAN) {
    GroupAddOption("tuning", 'f', "force",
              istring("remove the job from the local list of jobs "
                      "even if the job is not found in the infosys"),
              forceclean);
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

  if (cIsJobMan) {
    GroupAddOption("filtering", 's', "status",
              istring("only select jobs whose status is statusstr"),
              istring("statusstr"),
              status);

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

  if (c == CO_SUB || c == CO_TEST) {
    GroupAddOption("filtering", 'b', "broker",
              istring("select broker method (list available brokers with --listplugins flag)"),
              istring("broker"), broker);

    GroupAddOption("tuning", 'o', "jobids-to-file",
              istring("the IDs of the submitted jobs will be appended to this file"),
              istring("filename"),
              jobidoutfile);

    GroupAddOption("tuning", 'n', "no-delegation",
              istring("do not perform any delegation for submitted jobs"),
              no_delegation);

    GroupAddOption("tuning", 'X', "x509-delegation",
              istring("perform X.509 delegation for submitted jobs"),
              x509_delegation);

    GroupAddOption("tuning", 'K', "token-delegation",
              istring("perform token delegation for submitted jobs"),
              token_delegation);

    GroupAddOption("tuning", '\0', "instances-max",
              istring("request at most this number of job instances submitted in single submit request"),
              "", instances_max);

    GroupAddOption("tuning", '\0', "instances-min",
              istring("request at least this number of job instances submitted in single submit request"),
              "", instances_min);
  }

  if (cIsJobMan) {
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

    GroupAddOption("xaction", 'x', "dumpdescription",
              istring("do not submit - dump job description "
                      "in the language accepted by the target"),
              dumpdescription);
  }
  
  if (c == CO_TEST) {
    GroupAddOption("xaction", 'E', "certificate", istring("prints info about installed user- and CA-certificates"), show_credentials);
    GroupAddOption("tuning", '\0', "allowinsecureconnection", istring("allow TLS connection which failed verification"), allow_insecure_connection);
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

  /* --- Common options below --- */

  GroupAddOption("tuning", '\0', "no-authentication",
              istring("do not perform any authentication for opened connections"),
              no_authentication);

  GroupAddOption("tuning", '\0', "x509-authentication",
              istring("perform X.509 authentication for opened connections"),
              x509_authentication);

  GroupAddOption("tuning", '\0', "token-authentication",
              istring("perform token authentication for opened connections"),
              token_authentication);

  GroupAddOption("tuning", '\0', "systemca",
              istring("force using CA certificates configuration provided by OpenSSL"),
              force_system_ca);

  GroupAddOption("tuning", '\0', "gridca",
              istring("force using CA certificates configuration for Grid services (typically IGTF)"),
              force_grid_ca);

}

bool ClientOptions::getDelegationType(Arc::Logger& logger, Arc::UserConfig const& usercfg, DelegationType& delegation_type) const {
  delegation_type = UndefinedDelegation;
  if(no_delegation) {
    if(delegation_type != UndefinedDelegation) {
      logger.msg(Arc::ERROR, "Conflicting delegation types specified.");
      return false;
    }
    delegation_type = NoDelegation;
  }
  if(x509_delegation) {
    if(delegation_type != UndefinedDelegation) {
      logger.msg(Arc::ERROR, "Conflicting delegation types specified.");
      return false;
    }
    delegation_type = X509Delegation;
  }
  if(token_delegation) {
    if(delegation_type != UndefinedDelegation) {
      logger.msg(Arc::ERROR, "Conflicting delegation types specified.");
      return false;
    }
    delegation_type = TokenDelegation;
  }

  // If delegation is not specified try to guess it
  if(delegation_type == UndefinedDelegation) {
    if(!usercfg.OToken().empty()) {
      delegation_type = TokenDelegation;
    } else {
      delegation_type = X509Delegation;
    }
  }

  if(delegation_type == X509Delegation) {
    if (!checkproxy(usercfg)) {
      return 1;
    }
  } else if(delegation_type == TokenDelegation) {
    if (!checktoken(usercfg)) {
      return 1;
    }
  }

  return true;
}

bool ClientOptions::getAuthenticationType(Arc::Logger& logger, Arc::UserConfig const& usercfg, AuthenticationType& authentication_type) const {
  authentication_type = UndefinedAuthentication;
  if(no_authentication) {
    if(authentication_type != UndefinedAuthentication) {
      logger.msg(Arc::ERROR, "Conflicting authentication types specified.");
      return false;
    }
    authentication_type = NoAuthentication;
  }
  if(x509_authentication) {
    if(authentication_type != UndefinedAuthentication) {
      logger.msg(Arc::ERROR, "Conflicting authentication types specified.");
      return false;
    }
    authentication_type = X509Authentication;
  }
  if(token_authentication) {
    if(authentication_type != UndefinedAuthentication) {
      logger.msg(Arc::ERROR, "Conflicting authentication types specified.");
      return false;
    }
    authentication_type = TokenAuthentication;
  }

  if(authentication_type == X509Authentication) {
    if (!checkproxy(usercfg)) {
      return 1;
    }
  } else if(authentication_type == TokenAuthentication) {
    if (!checktoken(usercfg)) {
      return 1;
    }
  }

  return true;
}

