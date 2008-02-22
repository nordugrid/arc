// apstat.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <arc/misc/ClientInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/URL.h>
#include <arc/Logger.h>
#include <arc/misc/ClientTool.h>

class APInfoTool: public Arc::ClientTool {
 public:
  std::string proxy_path;
  std::string key_path;
  std::string cert_path;
  std::string ca_dir;
  std::string config_path;
  APInfoTool(int argc,char* argv[]):Arc::ClientTool("apinfo") {
    ProcessOptions(argc,argv,"P:K:C:A:c:");
  };
  virtual void PrintHelp(void) {
    std::cout<<"apinfo [-h] [-d debug_level] [-l logfile] [-P proxy_path] [-C certificate_path] [-K private_key_path] [-A CA_directory_path] [-c config_path] service_url"<<std::endl;
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
/*! A prototype command line tool for service status queries to any ARC1
  service through LIDI. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apinfo <service-URL>

  Arguments:
  <service-URL> The URL of ARC1 service.
*/
int main(int argc, char* argv[]){
  APInfoTool tool(argc,argv);
  if(!tool) return EXIT_FAILURE;
  try{
    if ((argc-tool.FirstOption())!=1)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::URL url(argv[tool.FirstOption()]);
    if(!url) throw(std::invalid_argument(std::string("Can't parse specified URL")));
    bool tls;
    if(url.Protocol() == "http") { tls=false; }
    else if(url.Protocol() == "https") { tls=true; }
    else throw(std::invalid_argument(std::string("URL contains unsupported protocol")));
    Arc::MCCConfig cfg;
    if(!tool.proxy_path.empty()) cfg.AddProxy(tool.proxy_path);
    if(!tool.key_path.empty()) cfg.AddPrivateKey(tool.key_path);
    if(!tool.cert_path.empty()) cfg.AddCertificate(tool.cert_path);
    if(!tool.ca_dir.empty()) cfg.AddCADir(tool.ca_dir);
    cfg.GetOverlay(tool.config_path);
    Arc::ClientSOAP client(cfg,url.Host(),url.Port(),tls,url.Path());
    Arc::InformationRequest inforequest;
    Arc::PayloadSOAP request(*(inforequest.SOAP()));
    Arc::PayloadSOAP* response;
    Arc::MCC_Status status = client.process(&request,&response);
    if(!status) {
      if(response) delete response;
      throw(std::runtime_error(std::string("Request failed")));
    };
    if(!response) {
      throw(std::runtime_error(std::string("There was no response")));
    };
    Arc::InformationResponse inforesponse(*response);
    if(!inforesponse) {
      std::string s;
      response->GetXML(s);
      delete response;
      std::cerr << "Wrong response: \n" << s << "\n" << std::endl;
      throw(std::runtime_error(std::string("Response is not valid")));
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
