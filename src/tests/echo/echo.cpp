#include <iostream>

#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/loader/MCCLoader.h"
#include "../../hed/PayloadSOAP.h"

#include "echo.h"


static Arc::Service* get_service(Arc::Config *cfg) {
    return new Echo::Service_Echo(cfg);
}

service_descriptor __arc_service_modules__[] = {
    { "echo", 0, &get_service }
};

using namespace Echo;


Service_Echo::Service_Echo(Arc::Config *cfg):Service(cfg) {
    ns_["echo"]="urn:echo";
    prefix_=(std::string)((*cfg)["prefix"]);
    suffix_=(std::string)((*cfg)["suffix"]);
}

Service_Echo::~Service_Echo(void) {
}

Arc::MCC_Status Service_Echo::make_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPMessage::SOAPFault* fault = outpayload->Fault();
  if(fault) {
    fault->Code(Arc::SOAPMessage::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status();
}

Arc::MCC_Status Service_Echo::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  // Both input and output are supposed to be SOAP 
  // Extracting payload
  Arc::PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) {
    std::cerr << "ECHO: input is not SOAP" << std::endl;
    return make_fault(outmsg);
  };
  // Analyzing request 
  Arc::XMLNode echo_op = (*inpayload)["echo"];
  if(!echo_op) {
    std::cerr << "ECHO: request is not supported - " << echo_op.Name() << std::endl;
    return make_fault(outmsg);
  };
  std::string say = echo_op["say"];
  std::string hear = prefix_+say+suffix_;
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
  outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=hear;
  outmsg.Payload(outpayload); 
  return Arc::MCC_Status();
}

