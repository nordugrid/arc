#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <sstream>

#include <arc/message/MCCLoader.h>
#include <arc/message/Service.h>
#include <arc/message/PayloadSOAP.h>

#include "echo.h"

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* mccarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new Echo::Service_Echo((Arc::Config*)(*mccarg),arg);
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "echo", "HED:SERVICE", NULL, 0, &get_service },
    { NULL, NULL, NULL, 0, NULL }
};

using namespace Arc;

namespace Echo {

Service_Echo::Service_Echo(Arc::Config *cfg, Arc::PluginArgument *parg):Service(cfg,parg),logger(Arc::Logger::rootLogger, "Echo") {
  ns_["echo"]="http://www.nordugrid.org/schemas/echo";
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

  // Assigning service description - Glue2 document should go here.
  infodoc.Assign(Arc::XMLNode(
    "<?xml version=\"1.0\"?>"
    "<Domains xmlns=\"http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01\"><AdminDomain><Services><Service><Endpoint><HealthState>ok</HealthState><ServingState>production</ServingState></Endpoint>ECHO</Service></Services></AdminDomain></Domains>"
  ),true);
}

Service_Echo::~Service_Echo(void) {
}

Arc::MCC_Status Service_Echo::make_fault(Arc::Message& outmsg,const std::string& txtmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload->Fault();
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    if(txtmsg.empty()) {
      fault->Reason("Failed processing request");
    } else {
      logger.msg(Arc::ERROR, txtmsg);
      fault->Reason(txtmsg);
    };
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
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
    return make_fault(outmsg,"Input is not SOAP");
  };
  {
      std::string str;
      inpayload->GetDoc(str, true);
      logger.msg(Arc::VERBOSE, "process: request=%s",str);
  }; 

  Arc::PayloadSOAP* outpayload = NULL;


  /**Export the formated policy-decision request**/
  MessageAuth* mauth = inmsg.Auth();
  MessageAuth* cauth = inmsg.AuthContext();
  if((!mauth) && (!cauth)) {
    logger.msg(ERROR,"Missing security object in message");
    return Arc::MCC_Status();
  };
  NS ns;
  XMLNode requestxml(ns,"");
  if(mauth) {
    if(!mauth->Export(SecAttr::ARCAuth,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return Arc::MCC_Status();
    };
  };
  if(cauth) {
    if(!cauth->Export(SecAttr::ARCAuth,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return Arc::MCC_Status();
    };
  };
  /** */

  // Analyzing request 

  // Checking if it's info request
  if((*inpayload)["size"]){
    Arc::XMLNode echo_op = (*inpayload)["size"];
    int size = atoi(std::string(echo_op["size"]).c_str());
    std::string msg = "Message for you, sir";
    msg.resize(size,'0');
    std::string say = echo_op["say"];
    std::string hear = prefix_+say+suffix_;
    outpayload = new Arc::PayloadSOAP(ns_);
    outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=msg;
  }
  else {
    // Then it must be 'echo' operation requested
    Arc::XMLNode echo_op = (*inpayload)["echo"];
    if(!echo_op) {
      return make_fault(outmsg,"Request is not supported - "+echo_op.Name());
    };
    std::string say = echo_op["say"];
    std::string hear = prefix_+say+suffix_;
    outpayload = new Arc::PayloadSOAP(ns_);
    outpayload->NewChild("echo:echoResponse").NewChild("echo:hear")=hear;
  };

  outmsg.Payload(outpayload);
  {
      std::string str;
      outpayload->GetDoc(str, true);
      logger.msg(Arc::VERBOSE, "process: response=%s",str);
  }; 
  return Arc::MCC_Status(Arc::STATUS_OK);
}


} // namespace Echo

