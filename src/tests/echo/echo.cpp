#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/security/SecHandler.h>

#include "echo.h"

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Echo::Service_Echo(cfg);
}

service_descriptors ARC_SERVICE_LOADER = {
    { "echo", 0, &get_service },
    { NULL, 0, NULL }
};

using namespace Echo;

//Arc::Logger Service_Echo::logger(Service::logger, "Echo");
//Arc::Logger Service_Echo::logger(Arc::Logger::getRootLogger(), "Echo");

Service_Echo::Service_Echo(Arc::Config *cfg):Service(cfg),logger(Arc::Logger::rootLogger, "Echo") {
  ns_["echo"]="urn:echo";
  prefix_=(std::string)((*cfg)["prefix"]);
  suffix_=(std::string)((*cfg)["suffix"]);  

#if 0
  // Parse the policy location information, and put them into a map container for later using
  for(int i=0;; i++) {
    Arc::XMLNode cn = (*cfg).Child(i);
    if(!cn) break;
    if(MatchXMLName(cn, "SecHandler")) {
      for(int j=0;; j++) {
        Arc::XMLNode gn = cn.Child(j);
        if(!gn) break;
        if(MatchXMLName(gn, "PDP")) {
          policylocation_  = (std::string)(gn.Attribute("policylocation"));
        }
      }
    }
  }
#endif
}

Service_Echo::~Service_Echo(void) {
}

Arc::MCC_Status Service_Echo::make_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload->Fault();
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::GENERIC_ERROR);
}

Arc::MCC_Status Service_Echo::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  //Store policy location into message attribute
  //inmsg.Attributes()->add("PDP:POLICYLOCATION", policylocation_);
  // Check authorization
  if(!ProcessSecHandlers(inmsg, "incoming")) {
    logger.msg(Arc::ERROR, "echo: Unauthorized");
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
  };
  // Both input and output are supposed to be SOAP 
  // Extracting payload
  Arc::PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) {
    logger.msg(Arc::ERROR, "Input is not SOAP");
    return make_fault(outmsg);
  };
  {
      std::string str;
      inpayload->GetDoc(str, true);
      logger.msg(Arc::DEBUG, "process: request=%s",str);
  }; 
  // Analyzing request 
  Arc::XMLNode echo_op = (*inpayload)["echo"];
  if(!echo_op) {
    logger.msg(Arc::ERROR, "Request is not supported - %s", echo_op.Name());
    return make_fault(outmsg);
  };
  std::string say = echo_op["say"];
  std::string hear = prefix_+say+suffix_;
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
  outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=hear;
  outmsg.Payload(outpayload);
  {
      std::string str;
      outpayload->GetDoc(str, true);
      logger.msg(Arc::DEBUG, "process: response=%s",str);
  }; 
  return Arc::MCC_Status(Arc::STATUS_OK);
}

