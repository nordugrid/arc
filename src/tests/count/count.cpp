#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string>

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/PayloadSOAP.h>

#include <arc/security/SecHandler.h>

#include "count.h"

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Count::Service_Count(cfg);
}

service_descriptors ARC_SERVICE_LOADER = {
    { "count", 0, &get_service },
    { NULL, 0, NULL }
};

using namespace Count;

Arc::Logger Service_Count::logger(Service::logger, "Count");

Service_Count::Service_Count(Arc::Config *cfg):Service(cfg) {
    ns_["count"]="urn:count";
}

Service_Count::~Service_Count(void) {
}

Arc::MCC_Status Service_Count::make_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload->Fault();
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::GENERIC_ERROR);
}

Arc::MCC_Status Service_Count::process(Arc::Message& inmsg,Arc::Message& outmsg) {

  std::cout<<"DN of the client: "<<inmsg.Attributes()->get("TLS:PEERDN")<<std::endl;

  Arc::PayloadSOAP* inpayload = NULL;
  try {
    inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
  } catch(std::exception& e) { };
  if(!inpayload) {
    logger.msg(Arc::ERROR, "Input is not SOAP");
    return make_fault(outmsg);
  };

  std::string xml;
  inpayload->GetXML(xml);
  std::cout << "request: "<< xml << std::endl;

  // Get operation
  Arc::XMLNode op = inpayload->Child(0);
  if(!op) {
    logger.msg(Arc::ERROR, "input does not define operation");
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
  };
  logger.msg(Arc::DEBUG, "process: operation: %s",op.Name().c_str());
  if(MatchXMLName(op, "minus")) {
    inmsg.Attributes()->set("COUNT:METHOD", "minus");
  }
  else if(MatchXMLName(op, "plus")) {
    inmsg.Attributes()->set("COUNT:METHOD", "plus");
  }
  else { 
    std::cout<<"Method is wrong"<<std::endl;
    return make_fault(outmsg);
  }

  std::cout<<"Count method is : "<<inmsg.Attributes()->get("COUNT:METHOD")<<std::endl;


  // Check authorization
  if(!ProcessSecHandlers(inmsg, "incoming")) {
    logger.msg(Arc::ERROR, "echo: Unauthorized");
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
  };
  printf("count_Authorized\n");


  // Analyzing request 
  Arc::XMLNode input1 = op["input1"];
  if(!input1) {
    logger.msg(Arc::ERROR, "Request is not supported - %s",
    input1.Name().c_str());
    return make_fault(outmsg);
  }

  Arc::XMLNode input2 = op["input2"];
  if(!input2) {
    logger.msg(Arc::ERROR, "Request is not supported - %s",
    input2.Name().c_str());
    return make_fault(outmsg);
  }

   
   std::string str1 = input1["number1"];
   std::string str2 = input2["number2"];

   int number1, number2, number3 = 0;

   std::istringstream ss1( str1 );
   ss1 >> number1;

   std::istringstream ss2( str2 );
   ss2 >> number2;

    if(MatchXMLName(op, "minus")) {
      number3 = number1 - number2;
   
    }
    else if(MatchXMLName(op, "plus")) {
      number3 = number1 + number2;
    }

   std::stringstream ss3;
   std::string str;
   ss3 << number3;
   ss3 >> str;
   

  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);

  outpayload->NewChild("count:countResponse") = str;
  outmsg.Payload(outpayload);
  std::cout << "elment" << std::endl;
  return Arc::MCC_Status(Arc::STATUS_OK);
}



