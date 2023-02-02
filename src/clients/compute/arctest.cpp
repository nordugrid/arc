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

#include <glibmm/fileutils.h>

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/compute/ComputingServiceRetriever.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobInformationStorage.h>
#include <arc/compute/SubmitterPlugin.h>
#include <arc/compute/JobDescription.h>
#include <arc/credential/Credential.h>
#include <arc/UserConfig.h>
#include <arc/compute/Broker.h>

#include "utils.h"
#include "submit.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");

int test(const Arc::UserConfig& usercfg, Arc::ExecutionTargetSorter& ets, const Arc::JobDescription& testJob, const std::string& jobidfile);

int dumpjobdescription_arctest_legacy(const Arc::UserConfig& usercfg, Arc::ExecutionTargetSorter& ets, const Arc::JobDescription& testJob);

static bool get_hash_value(const Arc::Credential& c, std::string& hash_str);

int RUNMAIN(arctest)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_TEST,
                    istring(" "),
                    istring("The arctest command is used for "
                            "testing clusters as resources."));

  std::list<std::string> params = opt.Parse(argc, argv);

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arctest", VERSION)
              << std::endl;
    return 0;
  }

  if ((opt.testjobid == -1) && (!opt.show_credentials) && (!opt.show_plugins)) {
    std::cout << Arc::IString("Nothing to do:\n"
        "you have to either specify a test job id with -J (--job)\n"
        "or query information about the certificates with -E (--certificate)\n");
    return 0;
  }

  if ((opt.testjobid == 1) && (!opt.runtime)) {
    std::cout << Arc::IString("For the 1st test job "
        "you also have to specify a runtime value with -r (--runtime) option.");
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
    showplugins("arctest", types, logger, usercfg.Broker().first);
    return 0;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  if (opt.show_credentials) {
    const Arc::Time now;

    std::cout << Arc::IString("Certificate information:") << std::endl;

    std::string certificate_issuer = "";
    if (usercfg.CertificatePath().empty()) {
      std::cout << "  " << Arc::IString("No user-certificate found") << std::endl << std::endl;
    } else {
      Arc::Credential holder(usercfg.CertificatePath(), "", usercfg.CACertificatesDirectory(), "");
      std::cout << "  " << Arc::IString("Certificate: %s", usercfg.CertificatePath()) << std::endl;
      if (!holder.GetDN().empty()) {
        std::cout << "  " << Arc::IString("Subject name: %s", holder.GetDN()) << std::endl;
        std::cout << "  " << Arc::IString("Valid until: %s", (std::string) holder.GetEndTime() ) << std::endl << std::endl;
        certificate_issuer = holder.GetIssuerName();
      }
      else {
        std::cout << "  " << Arc::IString("Unable to determine certificate information") << std::endl << std::endl;
      }
    }

    std::cout << Arc::IString("Proxy certificate information:") << std::endl;
    if (usercfg.ProxyPath().empty()) {
      std::cout << "  " << Arc::IString("No proxy found") << std::endl << std::endl;
    } else {
      Arc::Credential holder(usercfg.ProxyPath(), "", usercfg.CACertificatesDirectory(), "");
      std::cout << "  " << Arc::IString("Proxy: %s", usercfg.ProxyPath()) << std::endl;
      std::cout << "  " << Arc::IString("Proxy-subject: %s", holder.GetDN()) << std::endl;
      if (holder.GetEndTime() < now) {
        std::cout << "  " << Arc::IString("Valid for: Proxy expired") << std::endl << std::endl;
      } else if (!holder.GetVerification()) {
        std::cout << "  " << Arc::IString("Valid for: Proxy not valid") << std::endl << std::endl;
      } else {
        std::cout << "  " << Arc::IString("Valid for: %s", (holder.GetEndTime() - now).istr()) << std::endl << std::endl;
      }
    }

    if (!certificate_issuer.empty()) {
      std::cout << Arc::IString("Certificate issuer: %s", certificate_issuer) << std::endl << std::endl;
    }
    
    bool issuer_certificate_found = false;
    std::cout << Arc::IString("CA-certificates installed:") << std::endl;
    Glib::Dir cadir(usercfg.CACertificatesDirectory());
    for (Glib::DirIterator it = cadir.begin(); it != cadir.end(); ++it) {
      std::string cafile = Glib::build_filename(usercfg.CACertificatesDirectory(), *it);
      // Assume certificates have file ending ".0", ".1" or ".2". Very OpenSSL specific.
      if (Glib::file_test(cafile, Glib::FILE_TEST_IS_REGULAR) && (*it)[(*it).size()-2] == '.' &&
          ((*it)[(*it).size()-1] == '0' || (*it)[(*it).size()-1] == '1' || (*it)[(*it).size()-1] == '2')) {
        
        Arc::Credential cred(cafile, "", "", "");
        std::string dn = cred.GetDN();
        if (dn.empty()) continue;
          
        std::string hash;
        // Only accept certificates with correct hash.
        if (!get_hash_value(cred, hash) || hash != (*it).substr(0, (*it).size()-2)) continue;
        
        if (dn == certificate_issuer) issuer_certificate_found = true;
        std::cout << "  " << dn << std::endl;
      }
    }
    
    if (certificate_issuer.empty()) {
      std::cout << std::endl << Arc::IString("Unable to detect if issuer certificate is installed.") << std::endl;
    }
    else if (!issuer_certificate_found) {
      logger.msg(Arc::WARNING, "Your issuer's certificate is not installed");
    }

    return EXIT_SUCCESS;
  }

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if (!opt.broker.empty())
    usercfg.Broker(opt.broker);

  Arc::JobDescription testJob;
  if (!Arc::JobDescription::GetTestJob(opt.testjobid, testJob)) {
    std::cout << Arc::IString("No test-job, with ID \"%d\"", opt.testjobid) << std::endl;
    return 1;
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

  // Set user input variables into job description
  if (opt.testjobid == 1) {
    testJob.Application.Executable.Argument.back() = Arc::tostring(opt.runtime);
    testJob.Resources.TotalCPUTime = (opt.runtime+3)*60;
    for ( std::map<std::string, std::string>::iterator iter = testJob.OtherAttributes.begin();
      iter != testJob.OtherAttributes.end(); ++iter ) {
        char buffer [iter->second.length()+255];
        sprintf(buffer, iter->second.c_str(), opt.runtime, opt.runtime+3);
        iter->second = (std::string) buffer;
      }
  }

  // ARC6 target selection submission logic (same as arcsub)
  if ( opt.isARC6TargetSelectionOptions(logger) ) {
    // arctest only works with single test job in jobdescription list
    std::list<Arc::JobDescription> jobdescriptionlist;
    jobdescriptionlist.push_back(testJob);
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
  // legacy code that implements submission logic in arctest
  } else {
    Arc::Broker broker(usercfg, testJob, usercfg.Broker().first);
    if (!broker.isValid()) {
      logger.msg(Arc::ERROR, "Unable to load broker %s", usercfg.Broker().first);
      return 1;
    }
    logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

    std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.requestedSubmissionInterfaceName, opt.infointerface);
    std::set<std::string> preferredInterfaceNames;
    if (usercfg.InfoInterface().empty()) {
      preferredInterfaceNames.insert("org.nordugrid.ldapglue2");
    } else {
      preferredInterfaceNames.insert(usercfg.InfoInterface());
    }

    Arc::ExecutionTargetSorter ets(broker);

    std::list<std::string> rejectDiscoveryURLs = getRejectDiscoveryURLsFromUserConfigAndCommandLine(usercfg, opt.rejectdiscovery);
    Arc::ComputingServiceRetriever csr(usercfg, std::list<Arc::Endpoint>(), rejectDiscoveryURLs, preferredInterfaceNames);
    csr.addConsumer(ets);
    for (std::list<Arc::Endpoint>::const_iterator it = services.begin(); it != services.end(); ++it) {
      csr.addEndpoint(*it);
    }
    csr.wait();

    if (csr.empty()) {
      if (!opt.dumpdescription) {
        std::cout << Arc::IString("Test aborted because no resource returned any information") << std::endl;
      } else {
        std::cout << Arc::IString("Unable to adapt job description to any resource, no resource information could be obtained.") << std::endl;
        std::cout << Arc::IString("Original job description is listed below:") << std::endl;
        std::string descOutput;
        testJob.UnParse(descOutput, testJob.GetSourceLanguage());
        std::cout << descOutput << std::endl;
      }
      return 1;
    }

    if (ets.getMatchingTargets().empty()) {
      if (!opt.dumpdescription) {
        std::cout << Arc::IString("ERROR: Test aborted because no suitable resources were found for the test-job") << std::endl;
      } else {
        std::cout << Arc::IString("ERROR: Dumping job description aborted because no suitable resources were found for the test-job") << std::endl;
      }
      return 1;
    }

    if (opt.dumpdescription) {
       return dumpjobdescription_arctest_legacy(usercfg, ets, testJob);
    }
  
    std::cout << Arc::IString("Submitting test-job %d:", opt.testjobid) << std::endl;
    std::string testJobXRSL;
    testJob.UnParse(testJobXRSL, "nordugrid:xrsl");
    std::cout << testJobXRSL << std::endl;
    std::cout << Arc::IString("Client version: nordugrid-arc-%s", VERSION) << std::endl;
    return test(usercfg, ets, testJob, opt.jobidoutfile);
  }
}

void printjobid(const std::string& jobid, const std::string& jobidfile) {
  if (!jobidfile.empty())
    if (!Arc::Job::WriteJobIDToFile(jobid, jobidfile))
      logger.msg(Arc::WARNING, "Cannot write jobid (%s) to file (%s)", jobid, jobidfile);
  std::cout << Arc::IString("Test submitted with jobid: %s", jobid) << std::endl;
}

int test(const Arc::UserConfig& usercfg, Arc::ExecutionTargetSorter& ets, const Arc::JobDescription& testJob, const std::string& jobidfile) {
  int retval = 0;

  std::list<std::string> jobids;
  std::list<Arc::Job> submittedJobs;
  std::map<int, std::string> notsubmitted;

  submittedJobs.push_back(Arc::Job());

  for (ets.reset(); !ets.endOfList(); ets.next()) {
    if (ets->Submit(usercfg, testJob, submittedJobs.back())) {
      printjobid(submittedJobs.back().JobID, jobidfile);
      std::cout << Arc::IString("Computing service: %s", ets->ComputingService->Name) << std::endl;
      break;
    }
  }
  
  if (ets.endOfList()) {
    std::cout << Arc::IString("Test failed, no more possible targets") << std::endl;
    submittedJobs.pop_back();
    retval = 1;
  }

  Arc::JobInformationStorage *jobstore = createJobInformationStorage(usercfg);
  if (jobstore == NULL) {
    logger.msg(Arc::ERROR, "Unable to read job information from file (%s)", usercfg.JobListFile());
    return 1;
  }
  if (!jobstore->Write(submittedJobs)) {
    std::cout << Arc::IString("Warning: Failed to write job information to file (%s)", usercfg.JobListFile())
              << std::endl;
    std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
  }
  delete jobstore;

  return retval;
}

int dumpjobdescription_arctest_legacy(const Arc::UserConfig& usercfg, Arc::ExecutionTargetSorter& ets, const Arc::JobDescription& testJob) {
  for (ets.reset(); !ets.endOfList(); ets.next()) {
    Arc::JobDescription preparedTestJob(testJob);
    std::string jobdesc;
    // Prepare the test jobdescription according to the chosen ExecutionTarget
    if (!preparedTestJob.Prepare(*ets)) {
      logger.msg(Arc::INFO, "Unable to prepare job description according to needs of the target resource (%s).", ets->ComputingEndpoint->URLString); 
      continue;
    }
  
    std::string jobdesclang = "emies:adl";
    if (ets->ComputingEndpoint->InterfaceName == "org.nordugrid.gridftpjob") {
      jobdesclang = "nordugrid:xrsl";
    }
    
    if (!preparedTestJob.UnParse(jobdesc, jobdesclang)) {
      logger.msg(Arc::INFO, "An error occurred during the generation of job description to be sent to %s", ets->ComputingEndpoint->URLString); 
      continue;
    }
  
    std::cout << Arc::IString("Job description to be sent to %s:", ets->AdminDomain->Name) << std::endl;
    std::cout << jobdesc << std::endl;
    break;
  }

  return (!ets.endOfList());
}

static bool get_hash_value(const Arc::Credential& c, std::string& hash_str) {
  X509* cert = c.GetCert();
  if(!cert) return false;
  X509_NAME* cert_name = X509_get_subject_name(cert);
  if(!cert_name) return false;

  char hash[32];
  memset(hash, 0, 32);
  snprintf(hash, 32, "%08lx", X509_NAME_hash(cert_name));
  hash_str = hash;
  X509_free(cert);
  return true;
}
