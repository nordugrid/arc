#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/loader/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/Thread.h>

#include "pdpservice.h"

namespace ArcSec {

static Arc::LogStream logcerr(std::cerr);

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new Service_PDP(cfg);
}

Arc::MCC_Status Service_PDP::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Service_PDP::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");

  // Identify which of served endpoints request is for.
  // Service_PDP can only accept POST method
  if(method == "POST") {
    logger.msg(Arc::DEBUG, "process: POST");
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };
    // Analyzing request
    Arc::XMLNode request = inpayload->Child(0);
    if(!request) {
      logger.msg(Arc::ERROR, "input does not include any request");
      return make_soap_fault(outmsg);
    };
    std::string req_xml;
    request.GetXML(req_xml);
    logger.msg(Arc::DEBUG, "request: %s",req_xml.c_str());
    
    //Call the functionality of policy engine
    Response *resp = NULL;
    resp = eval->evaluate(request);
    logger.msg(Arc::INFO, "There is : %d subjects, which satisty at least one policy", (resp->getResponseItems()).size());
    ResponseList rlist = resp->getResponseItems();
    if(!(rlist.empty())) logger.msg(Arc::INFO, "Authorized");
    int size = rlist.size();
    int i;
    for(i = 0; i < size; i++){
      ResponseItem* item = rlist[i];
      RequestTuple* tp = item->reqtp;
      Subject::iterator it;
      Subject subject = tp->sub;
      for (it = subject.begin(); it!= subject.end(); it++){
        AttributeValue *attrval;
        RequestAttribute *attr;
        attr = dynamic_cast<RequestAttribute*>(*it);
        if(attr){
          attrval = (*it)->getAttributeValue();
          if(attrval) logger.msg(Arc::INFO, "%s", (attrval->encode()).c_str());
        }
      }
    }
    
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);   
    Arc::XMLNode response_doc(ns_);
    Arc::XMLNode res_body = response_doc.NewChild("Response");
    for(i = 0; i<size; i++){
      ResponseItem* item = rlist[i];
      res_body.NewChild(Arc::XMLNode((item->reqxml)).Child());
    } 

    if(resp)
      delete resp;
  
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
  } 
  else {
    delete inmsg.Payload();
    logger.msg(Arc::DEBUG, "process: %s: not supported",method.c_str());
    return Arc::MCC_Status(); 
  }
  return Arc::MCC_Status();
}

Service_PDP::Service_PDP(Arc::Config *cfg):Service(cfg), logger_(Arc::Logger::rootLogger, "PDP_Service"), eval(NULL) {
  logger_.addDestination(logcerr);
  // Define supported namespaces
  ns_["ra"]="http://www.nordugrid.org/ws/schemas/request-arc";

  Arc::XMLNode node(*cfg);
  Arc::XMLNode nd = node.GetRoot();
  Arc::Config topcfg(nd);
  eval = new ArcEvaluator(topcfg);
}

Service_PDP::~Service_PDP(void) {
  if(eval)
    delete eval;
  eval = NULL;
}

} // namespace ArcSec

service_descriptors ARC_SERVICE_LOADER = {
    { "pdp_service", 0, &ArcSec::get_service },
    { NULL, 0, NULL }
};

