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
#include <arc/OptionParser.h>

//! A prototype client for service status queries.
/*! A prototype command line tool for service status queries to any ARC1
  service through LIDI. In the name, "ap" means "Arc Prototype".
  
  Usage:
  arcinfo <service-URL>

  Arguments:
  <service-URL> The URL of ARC1 service.
*/

int main(int argc, char* argv[]){

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("service_url"));

  std::string proxy_path;
  options.AddOption('P', "proxy", istring("path to proxy file"),
		    istring("path"), proxy_path);

  std::string cert_path;
  options.AddOption('C', "certifcate", istring("path to certificate file"),
		    istring("path"), cert_path);

  std::string key_path;
  options.AddOption('K', "key", istring("path to private key file"),
		    istring("path"), key_path);

  std::string ca_dir;
  options.AddOption('A', "cadir", istring("path to CA directory"),
		    istring("directory"), ca_dir);

  std::string config_path;
  options.AddOption('c', "config", istring("path to config file"),
		    istring("path"), config_path);

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

  try{
    if (params.size()!=1)
      throw std::invalid_argument("Wrong number of arguments!");
    std::list<std::string>::iterator it = params.begin();
    Arc::URL url(*it);
    if(!url)
      throw std::invalid_argument("Can't parse specified URL");
    Arc::MCCConfig cfg;
    if(!proxy_path.empty()) cfg.AddProxy(proxy_path);
    if(!key_path.empty()) cfg.AddPrivateKey(key_path);
    if(!cert_path.empty()) cfg.AddCertificate(cert_path);
    if(!ca_dir.empty()) cfg.AddCADir(ca_dir);
    cfg.GetOverlay(config_path);
    Arc::ClientSOAP client(cfg,url);
    Arc::InformationRequest inforequest;
    Arc::PayloadSOAP request(*(inforequest.SOAP()));
    Arc::PayloadSOAP* response;
    Arc::MCC_Status status = client.process(&request,&response);
    if(!status) {
      if(response) delete response;
      throw std::runtime_error("Request failed");
    };
    if(!response) {
      throw std::runtime_error("There was no response");
    };
    Arc::InformationResponse inforesponse(*response);
    if(!inforesponse) {
      std::string s;
      response->GetXML(s);
      delete response;
      std::cerr << "Wrong response: \n" << s << "\n" << std::endl;
      throw std::runtime_error("Response is not valid");
    };
    std::list<Arc::XMLNode> results = inforesponse.Result();
    for(std::list<Arc::XMLNode>::iterator r = results.begin();r!=results.end();++r) {
      std::string s;
      r->GetXML(s,true);
      std::cout << "\n" << s << std::endl;
    };
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
