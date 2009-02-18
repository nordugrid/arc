#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/UserConfig.h>
#include <arc/OptionParser.h>

int main(int argc, char *argv[]) {

  setlocale(LC_ALL, "");

  Arc::Logger logger(Arc::Logger::getRootLogger(), "arcinfo");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("service_url"));

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

  std::list<std::string> params = options.Parse(argc, argv);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcinfo", VERSION) << std::endl;
    return 0;
  }

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
    std::cout << Arc::IString("%s version %s", "arcstat", VERSION)
	      << std::endl;
    return 0;
  }

  try {
    if (params.size() != 1)
      throw std::invalid_argument("Wrong number of arguments!");
    std::list<std::string>::iterator it = params.begin();
    Arc::URL url(*it);
    if (!url)
      throw std::invalid_argument("Can't parse specified URL");
    Arc::MCCConfig cfg;

    Arc::NS ns;
    Arc::XMLNode cred(ns, "cred");
    usercfg.ApplySecurity(cred);

    if (cred["ProxyPath"])
      cfg.AddProxy((std::string)cred["ProxyPath"]);
    if (cred["CertificatePath"])
      cfg.AddCertificate((std::string)cred["CertificatePath"]);
    if (cred["KeyPath"])
      cfg.AddPrivateKey((std::string)cred["KeyPath"]);
    if (cred["CACertificatesDir"])
      cfg.AddCADir((std::string)cred["CACertificatesDir"]);

    Arc::ClientSOAP client(cfg, url);
    Arc::InformationRequest inforequest;
    Arc::PayloadSOAP request(*(inforequest.SOAP()));
    Arc::WSAHeader(request).To(url.str());
    Arc::PayloadSOAP *response;
    Arc::MCC_Status status = client.process(&request, &response);
    if (!status) {
      if (response)
	delete response;
      throw std::runtime_error("Request failed");
    }
    if (!response)
      throw std::runtime_error("There was no response");
    Arc::InformationResponse inforesponse(*response);
    if (!inforesponse) {
      std::string s;
      response->GetXML(s);
      delete response;
      std::cerr << "Wrong response: \n" << s << "\n" << std::endl;
      throw std::runtime_error("Response is not valid");
    }
    std::list<Arc::XMLNode> results = inforesponse.Result();
    for (std::list<Arc::XMLNode>::iterator r = results.begin();
	 r != results.end(); ++r) {
      std::string s;
      r->GetXML(s, true);
      std::cout << "\n" << s << std::endl;
    }
    return EXIT_SUCCESS;
  }
  catch (std::exception& err) {
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
