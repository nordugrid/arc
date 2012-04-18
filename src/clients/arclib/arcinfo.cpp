// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/client/Endpoint.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ComputingServiceRetriever.h>

#include "utils.h"

int RUNMAIN(arcinfo)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcinfo");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  ClientOptions opt(ClientOptions::CO_INFO,
                    istring("[resource ...]"),
                    istring("The arcinfo command is used for "
                            "obtaining the status of computing "
                            "resources on the Grid."));

  {
    std::list<std::string> clusterstmp = opt.Parse(argc, argv);
    opt.clusters.insert(opt.clusters.end(), clusterstmp.begin(), clusterstmp.end());
  }

  if (opt.showversion) {
    std::cout << Arc::IString("%s version %s", "arcinfo", VERSION) << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!opt.debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(opt.debug));

  if (opt.show_plugins) {
    std::list<std::string> types;
    types.push_back("HED:ServiceEndpointRetrieverPlugin");
    types.push_back("HED:TargetInformationRetrieverPlugin");
    showplugins("arcinfo", types, logger);
    return 0;
  }

  Arc::UserConfig usercfg(opt.conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (opt.debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (opt.timeout > 0)
    usercfg.Timeout(opt.timeout);

  std::list<Arc::Endpoint> endpoints = getServicesFromUserConfigAndCommandLine(usercfg, opt.indexurls, opt.clusters, opt.requestedSubmissionInterfaceName);

  std::list<std::string> preferredInterfaceNames;
  if (usercfg.InfoInterface().empty()) {
    preferredInterfaceNames.push_back("org.nordugrid.ldapglue2");
    preferredInterfaceNames.push_back("org.ogf.emies");
  } else {
    preferredInterfaceNames.push_back(usercfg.InfoInterface());
  }

  std::list<std::string> rejectedURLs = usercfg.RejectedURLs();

  Arc::ComputingServiceRetriever csr(usercfg, endpoints, rejectedURLs, preferredInterfaceNames);
  csr.wait();
  std::list<Arc::ExecutionTarget> etList;
  Arc::ExecutionTarget::GetExecutionTargets(csr, etList);
  for (std::list<Arc::ExecutionTarget>::const_iterator it = etList.begin(); it != etList.end(); ++it) {
    it->SaveToStream(std::cout, opt.longlist);
  }
  return 0;
}
