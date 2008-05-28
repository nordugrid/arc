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

ArcSec::PDPConfigContext* Service_Echo::get_pdpconfig(Arc::Message& inmsg, std::string& id) {
  ArcSec::PDPConfigContext* config = NULL;
  Arc::MessageContextElement* mcontext = (*inmsg.Context())[id];
  if(mcontext) {
    try {
      config = dynamic_cast<ArcSec::PDPConfigContext*>(mcontext);
    } catch(std::exception& e) { };
  }
  if(config == NULL) {
    config = new ArcSec::PDPConfigContext();
    if(config) {
      inmsg.Context()->Add(id, config);
    } else {
      logger.msg(Arc::ERROR, "Failed to create context for pdp request and policy");
    }
  }

  std::string remotehost = inmsg.Attributes()->get("TCP:REMOTEHOST");
  std::string subject = inmsg.Attributes()->get("TLS:IDENTITYDN");
  std::string action = inmsg.Attributes()->get("HTTP:METHOD");

  // See the PDP.h for detailed explaination about this internal structure

  ArcSec::AuthzRequestSection section1, section2, section3;
  section1.value = remotehost;
  section1.type = "string";
  section2.value = subject;
  section2.type = "string";
  section3.value = action;
  section3.type = "string";

  ArcSec::AuthzRequest request;
  request.subject.push_back(section1);
  request.subject.push_back(section2);
  request.action.push_back(section3);

  config->SetRequestItem(request);

  return config;
}

Service_Echo::Service_Echo(Arc::Config *cfg):Service(cfg),logger(Arc::Logger::rootLogger, "Echo") {
  ns_["echo"]="urn:echo";
  prefix_=(std::string)((*cfg)["prefix"]);
  suffix_=(std::string)((*cfg)["suffix"]);  

  // Parse the pdp identifier and policy location information, and put them into a map container for later using
  for(int i=0;; i++) {
    Arc::XMLNode cn = (*cfg).Child(i);
    if(!cn) break;
    if(MatchXMLName(cn, "SecHandler")) {
      for(int j=0;; j++) {
        Arc::XMLNode gn = cn.Child(j);
        if(!gn) break;
        if(MatchXMLName(gn, "PDP")) {
          //"id" attribute of "PDP" node must be parsed here, because it will be used by ArcPDP as identifier
          // for getting PDPConfigContext
          std::string id = (std::string)(gn.Attribute("id"));
          std::string policylocation = (std::string)(gn.Attribute("policylocation"));
          pdpinfo_[id] = policylocation;
        }
      }
    }
  }
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
  // Parse the pdp request, and put them into msg context
  ArcSec::PDPConfigContext* config = NULL;
  std::map<std::string, std::string>::iterator it;
  for(it = pdpinfo_.begin(); it != pdpinfo_.end(); it++) {       
    config =  get_pdpconfig(inmsg, (std::string&)((*it).first));
    if(!config) {
      logger.msg(Arc::ERROR, "Can't get pdp request and policy information");
      return Arc::MCC_Status();
    }
    config->SetPolicyLocation((*it).second);
    // According to the requirement of some services, like Storage Service, which need to re-set
    // the policy location each time a new incoming message comes. 
    // Then in some place, the policylocation can be set as "PDP::POLICYLOCATION", and
    // here the pdp location can be re-set, or add.
    // config->SetPolicyLocation((std::string)(inmsg.Attributes()->get("PDP::POLICYLOCATION")));
    // config->AddPolicyLocation((std::string)(inmsg.Attributes()->get("PDP::POLICYLOCATION")));
  }

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
      inpayload->GetDoc(str, str);
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

