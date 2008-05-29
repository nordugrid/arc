#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

#include <arc/ArcLocation.h>
#include <arc/XMLNode.h>
#include <arc/OptionParser.h>

#include "arex_client.h"

//! A prototype client for job termination.
/*! A prototype command line tool for removing a job from an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apclean <JobID-file>

  Arguments:
  <JobID-file> The name of a file in which the Job ID is stored.

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

  Arc::OptionParser options(istring("[service_url] id_file"));

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
    std::cout << Arc::IString("%s version %s", "apclean", VERSION) << std::endl;
    return 0;
  }

  try{
    if ((params.size()!=2) && (params.size()!=1))
      throw std::invalid_argument("Wrong number of arguments!");
    std::list<std::string>::reverse_iterator it = params.rbegin();
    std::string jobidarg = *it++;
    std::string jobid;
    std::ifstream jobidfile(jobidarg.c_str());
    if (!jobidfile)
      throw std::invalid_argument("Could not open " + jobidarg);
    std::getline<char>(jobidfile, jobid, 0);
    if (!jobidfile)
      throw std::invalid_argument("Could not read Job ID from " + jobidarg);
    std::string urlstr;
    if(params.size() == 1) {
      Arc::XMLNode jobxml(jobid);
      if(!jobxml)
        throw std::invalid_argument("Could not process Job ID from " + jobidarg);
      urlstr=(std::string)(jobxml["Address"]); // TODO: clever service address extraction
    } else {
      urlstr=*it++;
    };
    if(urlstr.empty())
      throw std::invalid_argument("Missing service URL.");
    Arc::URL url(urlstr);
    if(!url)
      throw std::invalid_argument("Can't parse service URL " + urlstr);
    Arc::MCCConfig cfg;
    if(!proxy_path.empty()) cfg.AddProxy(proxy_path);
    if(!key_path.empty()) cfg.AddPrivateKey(key_path);
    if(!cert_path.empty()) cfg.AddCertificate(cert_path);
    if(!ca_dir.empty()) cfg.AddCADir(ca_dir);
    cfg.GetOverlay(config_path);
    Arc::AREXClient ac(url,cfg);
    ac.clean(jobid);
    std::cout << "The job was removed." << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
