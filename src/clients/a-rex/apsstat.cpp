#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

#include <arc/ArcLocation.h>
#include <arc/misc/OptionParser.h>

#include "arex_client.h"

//! A prototype client for service status queries.
/*! A prototype command line tool for job status queries to an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apsstat

  Configuration:
  Which A-REX service to use is specified in a configuration file. The
  configuration file also specifies how to set up the communication
  chain for the client. The location of the configuration file is
  specified by the environment variable ARC_AREX_CONFIG. If there is
  no such environment variable, the configuration file is assumed to
  be "arex_client.xml" in the current working directory.
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
    std::cout << Arc::IString("%s version %s", "apsstat", VERSION) << std::endl;
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
    Arc::AREXClient ac(url,cfg);
    std::cout << "Service status: \n" << ac.sstat() << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
