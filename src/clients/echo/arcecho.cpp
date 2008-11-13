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
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>

int main(int argc, char** argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcecho");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::rootLogger.setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("service message"),
                            istring("The arcecho command is a client for "
                                    "the ARC echo service."),
			    istring("The service argument is a URL to an ARC "
				    "echo service.\n"
				    "The message argument is the message the "
				    "service should return."));

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.xml)"),
                    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> args = options.Parse(argc, argv);

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
    std::cout << Arc::IString("%s version %s", "arcecho", VERSION)
              << std::endl;
    return 0;
  }

  if (args.size() != 2) {
    logger.msg(Arc::ERROR, "Wrong number of arguments!");
    return 1;
  } 

  std::list<std::string>::iterator it = args.begin();

  Arc::URL service = *it++;
  std::string message = *it++;

  Arc::MCCConfig cfg;
  if (usercfg.ConfTree()["ProxyPath"])
    cfg.AddProxy((std::string)usercfg.ConfTree()["ProxyPath"]);
  if (usercfg.ConfTree()["CertificatePath"])
    cfg.AddCertificate((std::string)usercfg.ConfTree()["CertificatePath"]);
  if (usercfg.ConfTree()["KeyPath"])
    cfg.AddPrivateKey((std::string)usercfg.ConfTree()["KeyPath"]);
  if (usercfg.ConfTree()["CACertificatesDir"])
    cfg.AddCADir((std::string)usercfg.ConfTree()["CACertificatesDir"]);

  Arc::ClientSOAP client(cfg, service.Host(), service.Port(),
			 service.Protocol() == "https", service.Path());

  std::string xml;

  Arc::NS ns("echo", "urn:echo");
  Arc::PayloadSOAP request(ns);
  request.NewChild("echo:echo").NewChild("echo:say") = message;

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

  std::string answer = (std::string)((*response)["echoResponse"]["hear"]);

  delete response;

  std::cout << answer << std::endl;

  return 0;
}
