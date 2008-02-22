// apstat.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include "arex_client.h"

class APSstatTool: public Arc::ClientTool {
 public:
  std::string proxy_path;
  std::string key_path;
  std::string cert_path;
  std::string ca_dir;
  std::string config_path;
  APSstatTool(int argc,char* argv[]):Arc::ClientTool("apsstat") {
    ProcessOptions(argc,argv,"P:K:C:A:c:");
  };
  virtual void PrintHelp(void) {
    std::cout<<"apsstat [-h] [-d debug_level] [-l logfile] [-P proxy_path] [-C certificate_path] [-K private_key_path] [-A CA_directory_path] [-c config_path] service_url"<<std::endl;
    std::cout<<"\tPossible debug levels are VERBOSE, DEBUG, INFO, WARNING, ERROR and FATAL"<<std::endl;
  };
  virtual bool ProcessOption(char option,char* option_arg) {
    switch(option) {
      case 'P': proxy_path=option_arg;; break;
      case 'K': key_path=option_arg; break;
      case 'C': cert_path=option_arg; break;
      case 'A': ca_dir=option_arg; break;
      case 'c': config_path=option_arg; break;
      default: {
        std::cerr<<"Error processing option: "<<(char)option<<std::endl;
        PrintHelp();
        return false;
      };
    };
    return true;
  };
};

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
  APSstatTool tool(argc,argv);
  if(!tool) return EXIT_FAILURE;
  try{
    if ((argc-tool.FirstOption())!=1)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::URL url(argv[tool.FirstOption()]);
    if(!url) throw(std::invalid_argument(std::string("Can't parse specified URL")));
    Arc::MCCConfig cfg;
    if(!tool.proxy_path.empty()) cfg.AddProxy(tool.proxy_path);
    if(!tool.key_path.empty()) cfg.AddPrivateKey(tool.key_path);
    if(!tool.cert_path.empty()) cfg.AddCertificate(tool.cert_path);
    if(!tool.ca_dir.empty()) cfg.AddCADir(tool.ca_dir);
    cfg.GetOverlay(tool.config_path);
    Arc::AREXClient ac(url,cfg);
    std::cout << "Service status: \n" << ac.sstat() << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
