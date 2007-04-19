#include <iostream>

#include "../../libs/common/ArcConfig.h"
#include "../../hed/libs/loader/Loader.h"

int main(void) {
  // Load service chain
  std::cout << "Creating service side chain" << std::endl;
  Arc::Config service_config("service.xml");
  if(!service_config) {
    std::cerr << "Failed to load service configuration" << std::endl;
    return -1;
  };
  Arc::Loader service_loader(&service_config);
  std::cout << "Service side MCCs are loaded" << std::endl;

  std::cout << "Creating client side chain" << std::endl;
  // Create client chain
  Arc::XMLNode client_doc("\
    <ArcConfig\
      xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\
      xmlns:tcp=\"http://www.nordugrid.org/schemas/ArcMCCTCP/2007\">\
     <Plugins><Name>mcctcp</Name></Plugins>\
     <Plugins><Name>mcchttp</Name></Plugins>\
     <Plugins><Name>mccsoap</Name></Plugins>\
     <Chain>\
      <Component name='tcp.client' id='tcp'><tcp:host>localhost</tcp:host><tcp:port>60000</tcp:port></Component>\
      <Component name='http.client' id='http'><next id='tcp'/><method>POST</method></Component>\
      <Component name='soap.client' id='soap' entry='soap'><next id='http'/></Component>\
     </Chain>\
    </ArcConfig>");
  Arc::Config client_config(client_doc);
  if(!client_config) {
    std::cerr << "Failed to load client configuration" << std::endl;
    return -1;
  };
  Arc::Loader client_loader(&client_config);
  std::cout << "Client side MCCs are loaded" << std::endl;
  Arc::MCC* client_entry = client_loader["soap"];
  if(!client_entry) {
    std::cerr << "Client chain does not have entry point" << std::endl;
    return -1;
  };

  // Create and send echo request

  return 0;
}
