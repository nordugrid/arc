#include <iostream>
#include <signal.h>

#include "../../libs/common/ArcConfig.h"
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/message/SOAPMessage.h"
#include "../../hed/libs/message/PayloadSOAP.h"

int main(void) {
  signal(SIGTTOU,SIG_IGN);
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
     <ModuleManager>\
        <Path>.libs/</Path>\
        <Path>../../hed/mcc/http/.libs/</Path>\
        <Path>../../hed/mcc/soap/.libs/</Path>\
        <Path>../../hed/mcc/tcp/.libs/</Path>\
     </ModuleManager>\
     <Plugins><Name>mcctcp</Name></Plugins>\
     <Plugins><Name>mcchttp</Name></Plugins>\
     <Plugins><Name>mccsoap</Name></Plugins>\
     <Chain>\
      <Component name='tcp.client' id='tcp'><tcp:Connect><tcp:Host>localhost</tcp:Host><tcp:Port>60000</tcp:Port></tcp:Connect></Component>\
      <Component name='http.client' id='http'><next id='tcp'/><Method>POST</Method><Endpoint>/</Endpoint></Component>\
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
  std::cout << "Creating and sending request" << std::endl;
  Arc::SOAPMessage::NS echo_ns; echo_ns["echo"]="urn:echo";
  Arc::PayloadSOAP req(echo_ns);
  req.NewChild("echo").NewChild("say")="HELLO";
  Arc::Message reqmsg;
  Arc::Message repmsg;
  reqmsg.Payload(&req);
  Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
  if(!status) {
    std::cerr << "Request fialed" << std::endl;
    return -1;
  };
  Arc::PayloadSOAP* resp = NULL;
  if(repmsg.Payload() == NULL) {
    std::cerr << "There isn no response" << std::endl;
    return -1;
  };
  try {
    resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
  } catch(std::exception&) { };
  if(resp == NULL) {
    std::cerr << "Response is not SOAP" << std::endl;
    return -1;
  };
  std::cout << "Response: " << (std::string)((*resp)["echoResponse"]["hear"]) << std::endl;
      
  return 0;
}
