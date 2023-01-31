// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>
#include <set>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/compute/Broker.h>
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/SubmissionStatus.h>
#include <arc/compute/Submitter.h>

#include "utils.h"
#include "submit.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");

int RUNMAIN(arcsub)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_SUB,
                    istring("[filename ...]"),
                    istring("The arcsub command is used for "
                            "submitting jobs to Grid enabled "
                            "computing\nresources."));

  std::list<std::string> params = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcsub", VERSION)
              << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(opt.debug));

  logger.msg(Arc::VERBOSE, "Running command: %s", opt.GetCommandWithArguments());

  Arc::UserConfig usercfg(opt.conffile, opt.joblist);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:SubmitterPlugin");
    types.push_back("HED:ServiceEndpointRetrieverPlugin");
    types.push_back("HED:TargetInformationRetrieverPlugin");
    types.push_back("HED:JobDescriptionParserPlugin");
    types.push_back("HED:BrokerPlugin");
    showplugins("arcsub", types, logger, usercfg.Broker().first);
    return 0;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if (!opt.broker.empty())
    usercfg.Broker(opt.broker);

  opt.jobdescriptionfiles.insert(opt.jobdescriptionfiles.end(),
                                 params.begin(), params.end());

  if (opt.jobdescriptionfiles.empty() && opt.jobdescriptionstrings.empty()) {
    logger.msg(Arc::ERROR, "No job description input specified");
    return 1;
  }

  std::list<Arc::JobDescription> jobdescriptionlist;

  // Loop over input job description files
  for (std::list<std::string>::iterator it = opt.jobdescriptionfiles.begin();
       it != opt.jobdescriptionfiles.end(); ++it) {

    std::ifstream descriptionfile(it->c_str());

    if (!descriptionfile) {
      logger.msg(Arc::ERROR, "Can not open job description file: %s", *it);
      return 1;
    }

    descriptionfile.seekg(0, std::ios::end);
    std::streamsize length = descriptionfile.tellg();
    descriptionfile.seekg(0, std::ios::beg);

    char *buffer = new char[length + 1];
    descriptionfile.read(buffer, length);
    descriptionfile.close();

    buffer[length] = '\0';
    std::list<Arc::JobDescription> jobdescs;
    Arc::JobDescriptionResult parseres = Arc::JobDescription::Parse((std::string)buffer, jobdescs);
    if (parseres) {
      for (std::list<Arc::JobDescription>::iterator itJ = jobdescs.begin();
           itJ != jobdescs.end(); ++itJ) {
        itJ->Application.DryRun = opt.dryrun;
        for (std::list<Arc::JobDescription>::iterator itJAlt = itJ->GetAlternatives().begin();
             itJAlt != itJ->GetAlternatives().end(); ++itJAlt) {
          itJAlt->Application.DryRun = opt.dryrun;
        }
      }

      jobdescriptionlist.insert(jobdescriptionlist.end(), jobdescs.begin(), jobdescs.end());
    }
    else {
      logger.msg(Arc::ERROR, "Invalid JobDescription:");
      std::cout << buffer << std::endl;
      delete[] buffer;
      std::cerr << parseres.str() << std::endl;
      return 1;
    }
    delete[] buffer;
  }

  //Loop over job description input strings
  for (std::list<std::string>::iterator it = opt.jobdescriptionstrings.begin();
       it != opt.jobdescriptionstrings.end(); ++it) {

    std::list<Arc::JobDescription> jobdescs;
    Arc::JobDescriptionResult parseres = Arc::JobDescription::Parse(*it, jobdescs);
    if (parseres) {
      for (std::list<Arc::JobDescription>::iterator itJ = jobdescs.begin();
           itJ != jobdescs.end(); ++itJ) {
        itJ->Application.DryRun = opt.dryrun;
        for (std::list<Arc::JobDescription>::iterator itJAlt = itJ->GetAlternatives().begin();
             itJAlt != itJ->GetAlternatives().end(); ++itJAlt) {
          itJAlt->Application.DryRun = opt.dryrun;
        }
      }

      jobdescriptionlist.insert(jobdescriptionlist.end(), jobdescs.begin(), jobdescs.end());
    }
    else {
      logger.msg(Arc::ERROR, "Invalid JobDescription:");
      std::cout << *it << std::endl;
      std::cerr << parseres.str() << std::endl;
      return 1;
    }
  }

  DelegationType delegation_type = UndefinedDelegation;
  if(opt.no_delegation) {
    if(delegation_type != UndefinedDelegation) {
      logger.msg(Arc::ERROR, "Conflicting delegation types specified.");
      return 1;
    }
    delegation_type = NoDelegation;
  }
  if(opt.x509_delegation) {
    if(delegation_type != UndefinedDelegation) {
      logger.msg(Arc::ERROR, "Conflicting delegation types specified.");
      return 1;
    }
    delegation_type = X509Delegation;
  }
  if(opt.token_delegation) {
    if(delegation_type != UndefinedDelegation) {
      logger.msg(Arc::ERROR, "Conflicting delegation types specified.");
      return 1;
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

  if ( opt.isARC6TargetSelectionOptions(logger) ) {
    // canonicalize endpoint types
    if (!opt.canonicalizeARC6InterfaceTypes(logger)) return 1;

    // get endpoint batches according to ARC6 target selection logic
    std::list<std::list<Arc::Endpoint> > endpoint_batches;
    bool info_discovery = prepare_submission_endpoint_batches(usercfg, opt, endpoint_batches);
  
    // add rejectdiscovery if defined
    if (!opt.rejectdiscovery.empty()) usercfg.AddRejectDiscoveryURLs(opt.rejectdiscovery);

    // action: dumpjobdescription
    if (opt.dumpdescription) {
        if (!info_discovery) {
          logger.msg(Arc::ERROR,"Cannot adapt job description to the submission target when information discovery is turned off");
          return 1;
        }
        // dump description only for priority submission interface, no fallbacks
        std::list<Arc::Endpoint> services = endpoint_batches.front();
        std::string req_sub_iface;
        if (!opt.submit_types.empty()) req_sub_iface = opt.submit_types.front();
        return dumpjobdescription(usercfg, jobdescriptionlist, services, req_sub_iface);
    }

    // default action: start submission cycle
    return submit_jobs(usercfg, endpoint_batches, info_discovery, opt.jobidoutfile, jobdescriptionlist, delegation_type);
  } else {
    // Legacy target selection submission logic
    std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.requestedSubmissionInterfaceName, opt.infointerface);

    if (!opt.direct_submission) {
      usercfg.AddRejectDiscoveryURLs(opt.rejectdiscovery);
    }

    if (opt.dumpdescription) {
      return dumpjobdescription(usercfg, jobdescriptionlist, services, opt.requestedSubmissionInterfaceName);
    }

    return legacy_submit(usercfg, jobdescriptionlist, services, opt.requestedSubmissionInterfaceName, opt.jobidoutfile, opt.direct_submission, delegation_type);
  }
}
