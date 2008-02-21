// apsub.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientTool.h>
#include "arex_client.h"


class APSubTool: public Arc::ClientTool {
 public:
  std::string proxy_path;
  std::string key_path;
  std::string cert_path;
  std::string ca_dir;
  APSubTool(int argc,char* argv[]):Arc::ClientTool("apsub") {
    ProcessOptions(argc,argv,"P:K:C:A:");
  };
  virtual void PrintHelp(void) {
    std::cout<<"apsub [-h] [-d debug_level] [-l logfile] [-P proxy_path] [-C certificate_path] [-K private_key_path] [-A CA_directory_path] service_url jsdl_file id_file"<<std::endl;
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


//! A prototype client for job submission.
/*! A prototype command line tool for job submission to an A-REX
  service. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apsub <Service-URL> <JSDL-file> <JobID-file>

  Arguments:
  <JSDL-file> The name of a file that contains the job specification
  in JSDL format.
  <JobID-file> The name of a file in which the Job ID will be stored.

*/
int main(int argc, char* argv[]){
  APSubTool tool(argc,argv);
  if(!tool) return EXIT_FAILURE;
  try{
    if ((argc-tool.FirstOption())!=3)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::URL url(argv[tool.FirstOption()]);
    if(!url) throw(std::invalid_argument(std::string("Can't parse specified URL")));
    Arc::MCCConfig cfg;
    if(!tool.proxy_path.empty()) cfg.AddProxy(tool.proxy_path);
    if(!tool.key_path.empty()) cfg.AddPrivateKey(tool.key_path);
    if(!tool.cert_path.empty()) cfg.AddCertificate(tool.cert_path);
    if(!tool.ca_dir.empty()) cfg.AddCADir(tool.ca_dir);
    Arc::AREXClient ac(url,cfg);
    std::string jobid;
    std::ifstream jsdlfile(argv[tool.FirstOption()+1]);
    if (!jsdlfile)
      throw std::invalid_argument(std::string("Could not open ")+
				  std::string(argv[1]));
    std::ofstream jobidfile(argv[tool.FirstOption()+2]);
    jobid = ac.submit(jsdlfile);
    if (!jsdlfile)
      throw std::invalid_argument(std::string("Failed when reading from ")+
				  std::string(argv[1]));
    jobidfile << jobid;
    if (!jobidfile)
      throw std::invalid_argument(std::string("Could not write Job ID to ")+
				  std::string(argv[2]));
    std::cout << "Submitted the job!" << std::endl;
    std::cout << "Job ID stored in: " << argv[2] << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& e){
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
