#include <iostream>

#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/loader/ServiceLoader.h"
#include "../../hed/libs/message/PayloadSOAP.h"

#include "arex.h"


static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ARex::ARexService(cfg);
}

service_descriptor __arc_service_modules__[] = {
    { "a-rex", 0, &get_service },
    { NULL, 0, NULL }
};

using namespace ARex;
 

Arc::MCC_Status ARexService::CreateActivity(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetActivityStatuses(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::TerminateActivities(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetActivityDocuments(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::GetFactoryAttributesDocument(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::StopAcceptingNewActivities(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::StartAcceptingNewActivities(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::ChangeActivityStatus(Arc::XMLNode& in,Arc::XMLNode& out) {
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::make_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload->Fault();
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status();
}


Arc::MCC_Status ARexService::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  // Identify which of served endpoints request is for
  // Using simplified algorithm - POST for SOAP messages,
  // GET and PUT for data transfer
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");
  std::string id = inmsg.Attributes()->get("PLEXER:EXTENSION");
  while(id[0] == '/') id=id.substr(1);
  std::string subpath;
  {
    std::string::size_type p = id.find('/');
    if(p != std::string::npos) {
      subpath = id.substr(p);
      id.resize(p);
      while(subpath[0] == '/') subpath=subpath.substr(1);
    };
  };
  if(method == "POST") {
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      std::cerr << "A-Rex: input is not SOAP" << std::endl;
      return make_fault(outmsg);
    };
    // Analyzing request
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      std::cerr << "A-Rex: input does not define operation" << std::endl;
      return make_fault(outmsg);
    };
    // Check if request is for top of tree (BES factory) or particular 
    // job (listing activity)
    if(id.empty()) {
      // Factory operations
      Arc::XMLNode res;
      if(MatchXMLName(op,"CreateActivity")) {
        CreateActivity(op,res);
      } else if(MatchXMLName(op,"GetActivityStatuses")) {
        GetActivityStatuses(op,res);
      } else if(MatchXMLName(op,"TerminateActivities")) {
        TerminateActivities(op,res);
      } else if(MatchXMLName(op,"GetActivityDocuments")) {
        GetActivityDocuments(op,res);
      } else if(MatchXMLName(op,"GetFactoryAttributesDocument")) {
        GetFactoryAttributesDocument(op,res);
      } else if(MatchXMLName(op,"StopAcceptingNewActivities")) {
        StopAcceptingNewActivities(op,res);
      } else if(MatchXMLName(op,"StartAcceptingNewActivities")) {
        StartAcceptingNewActivities(op,res);
      } else if(MatchXMLName(op,"ChangeActivityStatus")) {
        ChangeActivityStatus(op,res);
      } else {
        std::cerr << "A-Rex: request is not supported - " << op.Name() << std::endl;
        return make_fault(outmsg);
      };
      Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
      outpayload->NewChild(res);
      outmsg.Payload(outpayload);
    } else {
      // Listing operations for session directories
    };
  } else if(method == "GET") {
  } else if(method == "PUT") {
  } else if(method == "HEAD") {
  };
  return Arc::MCC_Status();
}
 
ARexService::ARexService(Arc::Config *cfg):Service(cfg) {
    ns_["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns_["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
}

ARexService::~ARexService(void) {
}

