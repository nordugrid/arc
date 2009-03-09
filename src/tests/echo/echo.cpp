#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <arc/message/MCCLoader.h>
#include <arc/infosys/RegisteredService.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/security/SecHandler.h>

#include "echo.h"

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* mccarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new Echo::Service_Echo((Arc::Config*)(*mccarg));
}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "echo", "HED:SERVICE", 0, &get_service },
    { NULL, NULL, 0, NULL }
};

using namespace Arc;

namespace Echo {

Service_Echo::Service_Echo(Arc::Config *cfg):RegisteredService(cfg),logger(Arc::Logger::rootLogger, "Echo") {
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

  // Assigning service description - Glue2 document should go here.
  infodoc.Assign(Arc::XMLNode(
    "<?xml version=\"1.0\"?>"
    "<Domains><AdminDomain><Services><Service>ECHO</Service></Services></AdminDomain></Domains>"
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
      logger.msg(Arc::DEBUG, "process: request=%s",str);
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
  if(MatchXMLNamespace(inpayload->Child(0),"http://docs.oasis-open.org/wsrf/rp-2")) {
    Arc::SOAPEnvelope* outxml = infodoc.Process(*inpayload);
    if(!outxml) {
      return make_fault(outmsg,"WSRF request processing failed");
    };
    outpayload = new Arc::PayloadSOAP(*outxml);
    delete outxml;
  } else {
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
      logger.msg(Arc::DEBUG, "process: response=%s",str);
  }; 
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace Echo

