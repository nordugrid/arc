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

#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/client/ComputingServiceRetriever.h>
#include <arc/client/Job.h>
#include <arc/client/SubmitterPlugin.h>
#include <arc/client/JobDescription.h>
#include <arc/credential/Credential.h>
#include <arc/UserConfig.h>
#include <arc/client/Broker.h>

#include "utils.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcsub");

int test(const Arc::UserConfig& usercfg, Arc::ExecutionTargetSorter& ets, const Arc::JobDescription& testJob, const std::string& jobidfile);
int dumpjobdescription(const Arc::UserConfig& usercfg, Arc::ExecutionTargetSorter& ets, const Arc::JobDescription& testJob);

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
    std::cout << Arc::IString("For the 1st test job"
        "you also have to specify a runtime value with -r (--runtime) option.");
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));


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
    types.push_back("HED:JobDescriptionParser");
    types.push_back("HED:BrokerPlugin");
    showplugins("arctest", types, logger, usercfg.Broker().first);
    return 0;
  }

  if (!checkproxy(usercfg)) {
    return 1;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  if (!opt.broker.empty())
    usercfg.Broker(opt.broker);

  if (opt.show_credentials) {
    std::string proxy_path = usercfg.ProxyPath();
    std::string cert_path = usercfg.CertificatePath();
    std::string key_path = usercfg.KeyPath();
    std::string ca_dir = usercfg.CACertificatesDirectory();

    const Arc::Time now;

    std::cout << Arc::IString("Certificate information:") << std::endl << std::endl;

    if (cert_path.empty() || key_path.empty()) {
      std::cout << Arc::IString("No user-certificate found") << std::endl << std::endl;
    } else {
      Arc::Credential holder(cert_path, "", ca_dir, "");
      std::cout << Arc::IString("Certificate: %s", cert_path) << std::endl;
      std::cout << Arc::IString("Subject name: %s", holder.GetDN()) << std::endl;
      std::cout << Arc::IString("Valid until: %s", (std::string) holder.GetEndTime() ) << std::endl << std::endl;
    }

    if (proxy_path.empty()) {
      std::cout << Arc::IString("No proxy found") << std::endl << std::endl;
    } else {
      Arc::Credential holder(proxy_path, "", ca_dir, "");
      std::cout << Arc::IString("Proxy: %s", proxy_path) << std::endl;
      std::cout << Arc::IString("Proxy-subject: %s", holder.GetDN()) << std::endl;
      if (holder.GetEndTime() < now) {
        std::cout << Arc::IString("Valid for: Proxy expired") << std::endl << std::endl;
      } else if (!holder.GetVerification()) {
        std::cout << Arc::IString("Valid for: Proxy not valid") << std::endl << std::endl;
      } else {
        std::cout << Arc::IString("Valid for: %s", (holder.GetEndTime() - now).istr()) << std::endl << std::endl;
      }
    }

    if (!cert_path.empty() && !key_path.empty()) {
      Arc::Credential holder(cert_path, "", ca_dir, "");
      std::cout << Arc::IString("Certificate issuer: %s", holder.GetIssuerName()) << std::endl << std::endl; //TODO
    }

    return EXIT_SUCCESS;
  }

  Arc::JobDescription testJob;
  if (!Arc::JobDescription::GetTestJob(opt.testjobid, testJob)) {
    std::cout << Arc::IString("No test-job, with ID \"%d\"", opt.testjobid) << std::endl;
    return 1;
  }

  // Set user input variables into job description
  if (opt.testjobid == 1) {
    for ( std::map<std::string, std::string>::iterator iter = testJob.OtherAttributes.begin();
      iter != testJob.OtherAttributes.end(); ++iter ) {
        char buffer [iter->second.length()+255];
        sprintf(buffer, iter->second.c_str(), opt.runtime+1, opt.runtime+3);
        iter->second = (std::string) buffer;
      }
  }

  Arc::Broker broker(usercfg, testJob, usercfg.Broker().first);
  if (!broker.isValid()) {
    logger.msg(Arc::ERROR, "Unable to load broker %s", usercfg.Broker().first);
    return 1;
  }
  logger.msg(Arc::INFO, "Broker %s loaded", usercfg.Broker().first);

  std::list<Arc::Endpoint> services = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.requestedSubmissionInterfaceName);
  std::list<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.push_back("org.nordugrid.ldapglue2");
  } else {
    preferredInterfaceNames.push_back(usercfg.InfoInterface());
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
      std::cout << Arc::IString("Dumping job description aborted because no resource returned any information") << std::endl;
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
     return dumpjobdescription(usercfg, ets, testJob);
  }
  return test(usercfg, ets, testJob, opt.jobidoutfile);
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
      printjobid(submittedJobs.back().JobID.fullstr(), jobidfile);
      break;
    }
  }
  
  if (ets.endOfList()) {
    std::cout << Arc::IString("Test failed, no more possible targets") << std::endl;
    submittedJobs.pop_back();
    retval = 1;
  }

  if (!Arc::Job::WriteJobsToFile(usercfg.JobListFile(), submittedJobs)) {
    std::cout << Arc::IString("Warning: Failed to lock job list file %s", usercfg.JobListFile())
              << std::endl;
    std::cout << Arc::IString("To recover missing jobs, run arcsync") << std::endl;
  }

  return retval;
}

int dumpjobdescription(const Arc::UserConfig& usercfg, Arc::ExecutionTargetSorter& ets, const Arc::JobDescription& testJob) {
  for (ets.reset(); !ets.endOfList(); ets.next()) {
    Arc::JobDescription preparedTestJob(testJob);
    std::string jobdesc;
    // Prepare the test jobdescription according to the choosen ExecutionTarget
    if (!preparedTestJob.Prepare(*ets)) {
      logger.msg(Arc::INFO, "Unable to prepare job description according to needs of the target resource (%s).", ets->ComputingEndpoint->URLString); 
      continue;
    }
  
    std::string jobdesclang = "nordugrid:jsdl";
    if (ets->ComputingEndpoint->InterfaceName == "org.nordugrid.gridftpjob") {
      jobdesclang = "nordugrid:xrsl";
    }
    else if (ets->ComputingEndpoint->InterfaceName == "org.glite.cream") {
      jobdesclang = "egee:jdl";
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
