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

Arc::Logger Service_Echo::logger(Service::logger, "Echo");

Service_Echo::Service_Echo(Arc::Config *cfg):Service(cfg) {
    ns_["echo"]="urn:echo";
    prefix_=(std::string)((*cfg)["prefix"]);
    suffix_=(std::string)((*cfg)["suffix"]);
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
  // Check authorization
  std::list<ArcSec::SecHandler*> hlist=sechandlers_["incoming"];
  std::list<ArcSec::SecHandler*>::iterator it;
  for(it=hlist.begin(); it!=hlist.end(); it++){
    ArcSec::SecHandler* h = *it;
    if(h->Handle(&inmsg)) break;
  }
  // The "Handle" method only returns true/false; The MCC/Service doesn't 
  // care about security process. "SecHandler" uses msg.attributes to 
  // exchange security related information
  if((it==hlist.end()) && (hlist.size() > 0)){
    printf("echo_UnAuthorized\n");
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
  } //Do we need to add some status in MCC_Status
  printf("echo_Authorized\n");

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
  // Analyzing request 
  Arc::XMLNode echo_op = (*inpayload)["echo"];
  if(!echo_op) {
    logger.msg(Arc::ERROR, "Request is not supported - %s",
	       echo_op.Name().c_str());
    return make_fault(outmsg);
  };
  std::string say = echo_op["say"];
  std::string hear = prefix_+say+suffix_;
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
  outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=hear;
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

