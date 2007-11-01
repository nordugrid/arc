// apstat.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <arc/misc/ClientInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/URL.h>

//! A prototype client for service status queries.
/*! A prototype command line tool for service status queries to any ARC1
  service through LIDI. In the name, "ap" means "Arc Prototype".
  
  Usage:
  apinfo <service-URL>

  Arguments:
  <service-URL> The URL of ARC1 service.
*/
int main(int argc, char* argv[]){
  try{
    if (argc!=2)
      throw std::invalid_argument("Wrong number of arguments!");
    Arc::URL url(argv[1]);
    if(!url) throw(std::invalid_argument(std::string("Can't parse specified URL")));
    bool tls;
    if(url.Protocol() == "http") { tls=false; }
    else if(url.Protocol() == "https") { tls=true; }
    else throw(std::invalid_argument(std::string("URL contains unsupported protocol")));
    Arc::BaseConfig cfg;
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
    std::string s;
    response->GetXML(s);
    std::cout << "Response:\n" << s << std::endl;
    return EXIT_SUCCESS;
  }
  catch (std::exception& err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}
