// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/UserConfig.h>

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "srmping");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::rootLogger.setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("service"),
                            istring("The arcsrmping command is a ping client "
                                    "for the SRM service."),
                            istring("The service argument is a URL to an SRM "
                                    "service."));

  int timeout = -1;
  options.AddOption('t', "timeout", istring("timeout in seconds (default " + Arc::tostring(Arc::UserConfig::DEFAULT_TIMEOUT) + ")"),
                    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.conf)"),
                    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> args = options.Parse(argc, argv);

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }
  
  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));
  
  if (timeout > 0)
    usercfg.Timeout(timeout);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcsrmping", VERSION)
              << std::endl;
    return 0;
  }

  if (args.size() != 1) {
    logger.msg(Arc::ERROR, "Wrong number of arguments!");
    return 1;
  }

  std::list<std::string>::iterator it = args.begin();

  Arc::URL service = *it;

  if (service.Protocol() == "srm")
    service.ChangeProtocol("httpg");

  Arc::MCCConfig cfg;
  usercfg.ApplyToConfig(cfg);

  Arc::ClientSOAP client(cfg, service, usercfg.Timeout());

  std::string xml;

  Arc::NS ns("SRMv2", "http://srm.lbl.gov/StorageResourceManager");
  Arc::PayloadSOAP request(ns);
  request.NewChild("SRMv2:srmPing").NewChild("srmPingRequest");

  request.GetXML(xml, true);
  logger.msg(Arc::INFO, "Request:\n%s", xml);

  Arc::PayloadSOAP *response = NULL;

  Arc::MCC_Status status = client.process(&request, &response);

  if (!status) {
    logger.msg(Arc::ERROR, (std::string)status);
    if (response)
      delete response;
    return 1;
  }

  if (!response) {
    logger.msg(Arc::ERROR, "No SOAP response");
    return 1;
  }

  response->GetXML(xml, true);
  logger.msg(Arc::INFO, "Response:\n%s", xml);

  Arc::XMLNode pingresponse = (*response)["srmPingResponse"]["srmPingResponse"];

  std::string srmversion = (std::string)(pingresponse["versionInfo"]);

  std::cout << "SRM version: " << srmversion << std::endl;

  for (Arc::XMLNode n = pingresponse["otherInfo"]["extraInfoArray"]; n; ++n)
    std::cout << "  " << (std::string)n["key"]
              << ": " << (std::string)n["value"] << std::endl;

  delete response;

  return 0;
}
