// apkill.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include <arc/XMLNode.h>
#include "arex_client.h"

class APKillTool: public Arc::ClientTool {
 public:
  std::string proxy_path;
  std::string key_path;
  std::string cert_path;
  std::string ca_dir;
  APKillTool(int argc,char* argv[]):Arc::ClientTool("apkill") {
    ProcessOptions(argc,argv,"P:K:C:A:");
  };
  virtual void PrintHelp(void) {
    std::cout<<"apkill [-h] [-d debug_level] [-l logfile] [-P proxy_path] [-C certificate_path] [-K private_key_path] [-A CA_directory_path] [service_url] id_file"<<std::endl;
    std::cout<<"\tPossible debug levels are VERBOSE, DEBUG, INFO, WARNING, ERROR and FATAL"<<std::endl;
  };
  virtual bool ProcessOption(char option,char* option_arg) {
    switch(option) {
      case 'P': proxy_path=option_arg;; break;
      case 'K': key_path=option_arg; break;
      case 'C': cert_path=option_arg; break;
      case 'A': ca_dir=option_arg; break;
      default: {
        std::cerr<<"Error processing option: "<<(char)option<<std::endl;
        PrintHelp();
        return false;
      };
    };
    return true;
  };
};

//! A prototype client for job termination.
/*! A prototype command line tool for terminating a job of an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apkill <JobID-file>

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
  APKillTool tool(argc,argv);
  if(!tool) return EXIT_FAILURE;
  try{
    if (((argc-tool.FirstOption())!=2) && ((argc-tool.FirstOption())!=1))
      throw std::invalid_argument("Wrong number of arguments!");
    char* jobidarg = argv[tool.FirstOption()];
    if((argc-tool.FirstOption()) == 2) jobidarg=argv[tool.FirstOption()+1];;
    std::string jobid;
    std::ifstream jobidfile(jobidarg);
    if (!jobidfile)
      throw std::invalid_argument(std::string("Could not open ")+jobidarg);
    std::getline<char>(jobidfile, jobid, 0);
    if (!jobidfile)
      throw std::invalid_argument(std::string("Could not read Job ID from ")+jobidarg);
    std::string urlstr;
    if((argc-tool.FirstOption()) == 1) {
      Arc::XMLNode jobxml(jobid);
      if(!jobxml)
        throw std::invalid_argument(std::string("Could not process Job ID from ")+jobidarg);
      urlstr=(std::string)(jobxml["Address"]); // TODO: clever service address extraction
    } else {
      urlstr=argv[tool.FirstOption()];
    };
    if(urlstr.empty())
      throw std::invalid_argument("Missing service URL.");
    Arc::URL url(urlstr);
    if(!url)
      throw std::invalid_argument(std::string("Can't parse service URL ")+urlstr);
    Arc::MCCConfig cfg;
    if(!tool.proxy_path.empty()) cfg.AddProxy(tool.proxy_path);
    if(!tool.key_path.empty()) cfg.AddPrivateKey(tool.key_path);
    if(!tool.cert_path.empty()) cfg.AddCertificate(tool.cert_path);
    if(!tool.ca_dir.empty()) cfg.AddCADir(tool.ca_dir);
    Arc::AREXClient ac(url,cfg);
    ac.kill(jobid);
    std::cout << "The job was terminated." << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
