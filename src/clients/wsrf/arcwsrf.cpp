// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <string>

#include <arc/ArcLocation.h>
#include <arc/Utils.h>
#include <arc/IString.h>
#include <arc/Logger.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/message/MCC.h>
#include <arc/communication/ClientInterface.h>

using namespace Arc;
#ifdef TEST
#define RUN(X) test_arcwsrf_##X
#else
#define RUN(X) X
#endif
int RUN(main)(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcwsrf");
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  //Arc::ArcLocation::Init(argv[0]);
  Arc::OptionParser options(istring("URL [query]"),
                            istring("The arcwsrf command is used for "
                                    "obtaining the WS-ResourceProperties of\n"
                                    "services."));

  std::list<std::string> paths;
  options.AddOption('p', "property",
                    istring("Request for specific Resource Property"),
                    istring("[-]name"),
                    paths);

  int timeout = -1;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
                    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.conf)"),
                    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::string url;
  std::string query;
  {
    std::list<std::string> args = options.Parse(argc, argv);

    if (version) {
      std::cout << Arc::IString("%s version %s", "arcwsrf", VERSION)
                << std::endl;
      return 0;
    }

    if(args.size() < 1) {
      logger.msg(Arc::ERROR, "Missing URL");
      return 1;
    }

    if(url.size() > 2) {
      logger.msg(Arc::ERROR, "Too many parameters");
      return 1;
    }

    std::list<std::string>::iterator arg = args.begin();
    url = *arg;
    ++arg;
    if(arg != args.end()) query = *arg;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(debug));

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (timeout > 0)
    usercfg.Timeout(timeout);

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  //if (timeout > 0) {
  //  usercfg.SetTimeout(timeout);
  //}

  // Proxy check
  //if (!usercfg.CheckProxy())
  //  return 1;

  CountedPointer<InformationRequest> request(NULL);
  if(!query.empty()) {
    XMLNode q(query);
    if(!q) {
      logger.msg(Arc::ERROR, "Query is not a valid XML");
      return 1;
    }
    request = new InformationRequest(q);
  } else {
    std::list<std::list<std::string> > qpaths;
    for(std::list<std::string>::iterator path = paths.begin();
                                         path != paths.end();++path) {
      std::list<std::string> qpath;
      qpath.push_back(*path);
      qpaths.push_back(qpath);
    }
    request = new InformationRequest(qpaths);
  }
  if(!(*request)) {
    logger.msg(Arc::ERROR, "Failed to create WSRP request");
    return 1;
  }
  //SOAPEnvelope* SOAP(void);

  URL u(url);
  if(!u) {
    logger.msg(Arc::ERROR, "Specified URL is not valid");
    return 1;
  }

  MCCConfig cfg;
  usercfg.ApplyToConfig(cfg);
  ClientSOAP client(cfg, u, usercfg.Timeout());
  PayloadSOAP req(*(request->SOAP()));
  PayloadSOAP* resp_ = NULL;
  MCC_Status r = client.process(&req,&resp_);
  CountedPointer<PayloadSOAP> resp(resp_);
  if(!r) {
    logger.msg(Arc::ERROR, "Failed to send request");
    return 1;
  }
  if(!resp) {
    logger.msg(Arc::ERROR, "Failed to obtain SOAP response");
    return 1;
  }
  std::string o;
  resp->Body().Child().GetXML(o);
  if(resp->IsFault()) {
    logger.msg(Arc::ERROR, "SOAP fault received");
    std::cerr<<o<<std::endl;
    return 1;
  }
  std::cout<<o<<std::endl;
  return 0;
}