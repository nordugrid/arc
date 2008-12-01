#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/security/ArcPDP/Source.h>
#include <arc/Thread.h>

#include "charon.h"

namespace ArcSec {

static Arc::LogStream logcerr(std::cerr);

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new Charon((Arc::Config*)(*mccarg));
}

Arc::MCC_Status Charon::make_soap_fault(Arc::Message& outmsg) {
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status Charon::process(Arc::Message& inmsg,Arc::Message& outmsg) {
  
  std::string method = inmsg.Attributes()->get("HTTP:METHOD");

  // Identify which of served endpoints request is for.
  // Charon can only accept POST method
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
    Arc::XMLNode request = (*inpayload)["GetPolicyDecisionRequest"];
    if(!request) {
      logger.msg(Arc::ERROR, "soap body does not include any request node");
      return make_soap_fault(outmsg);
    };
    {
      std::string req_xml;
      request.GetXML(req_xml);
      logger.msg(Arc::DEBUG, "Request: %s",req_xml);
    };    

    Arc::XMLNode arc_requestnd = request["Request"];
    if(!arc_requestnd) {
      logger.msg(Arc::ERROR, "request node is empty");
      return make_soap_fault(outmsg);
    };

    //Call the functionality of policy engine
    //Here the decision algorithm is the same as that in ArcPDP.cpp
    Response *resp = NULL;
    resp = eval->evaluate(Source(arc_requestnd));
    ResponseList rlist = resp->getResponseItems();
    int size = rlist.size();
    int i;

    bool atleast_onedeny = false;
    bool atleast_onepermit = false;

    for(i = 0; i < size; i++){
      ResponseItem* item = rlist[i];
      RequestTuple* tp = item->reqtp;

      if(item->res == DECISION_DENY)
        atleast_onedeny = true;
      if(item->res == DECISION_PERMIT)
        atleast_onepermit = true;

      Subject::iterator it;
      Subject subject = tp->sub;
      for (it = subject.begin(); it!= subject.end(); it++){
        AttributeValue *attrval;
        RequestAttribute *attr;
        attr = dynamic_cast<RequestAttribute*>(*it);
        if(attr){
          attrval = (*it)->getAttributeValue();
          if(attrval) logger.msg(Arc::INFO, "%s", (attrval->encode()));
        }
      }
    }

    bool result = false;
    if(atleast_onedeny) result = false;
    else if(!atleast_onedeny && atleast_onepermit) result = true;
    else if(!atleast_onedeny && !atleast_onepermit) result = false;

    if(result) logger.msg(Arc::INFO, "Authorized from Charon service");
    else logger.msg(Arc::ERROR, "UnAuthorized from Charon service; Some of the RequestItem does not satisfy Policy");

    //Put those request items which satisfy the policies into the response SOAP message (implicitly
    //means the decision result: <ItemA, permit>, <ItemB, permit>, <ItemC, deny> (ItemC will not in the 
    //response SOAP message)
    //The client of the pdpservice (normally a policy enforcement point, like job executor) is supposed
    //to compose the policy decision request to pdpservice by parsing the information from the request 
    //(aiming to the client itself), and permit or deny the request by using the information responded 
    //from pdpservice; Here pdpservice only gives some coarse decision to pdpservice invoker.
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);   
    Arc::XMLNode response = outpayload->NewChild("pdp:GetPolicyDecisionResponse");

    Arc::XMLNode arc_responsend = response.NewChild("response:Response");
    Arc::XMLNode authz_res = arc_responsend.NewChild("response:AuthZResult");
    if(result) authz_res = "PERMIT";
    else authz_res = "DENY";

    for(i = 0; i<size; i++){
      ResponseItem* item = rlist[i];
      if(item->res == DECISION_PERMIT)
        arc_responsend.NewChild(Arc::XMLNode(item->reqxml));
    } 

    if(resp) delete resp;
    
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
  } 
  else {
    delete inmsg.Payload();
    logger.msg(Arc::DEBUG, "process: %s: not supported",method);
    return Arc::MCC_Status(); 
  }
  return Arc::MCC_Status();
}

Charon::Charon(Arc::Config *cfg):Service(cfg), logger_(Arc::Logger::rootLogger, "Charon"), eval(NULL) {
  logger_.addDestination(logcerr);
  // Define supported namespaces
  ns_["ra"]="http://www.nordugrid.org/schemas/request-arc";
  ns_["response"]="http://www.nordugrid.org/schemas/response-arc";
  ns_["pdp"]="http://www.nordugrid.org/schemas/pdp";

  //Load the Evaluator object
  std::string evaluator = (*cfg)["PDPConfig"]["Evaluator"].Attribute("name");
  logger.msg(Arc::INFO, "Evaluator: %s", evaluator);
  ArcSec::EvaluatorLoader eval_loader;
  eval = eval_loader.getEvaluator(evaluator);
  if(eval == NULL) {
    logger.msg(Arc::ERROR, "Can not dynamically produce Evaluator");
  }
  else logger.msg(Arc::INFO, "Succeeded to produce Evaluator");

  //Get the policy location, and put it into evaluator
  Arc::XMLNode policyfile;
  Arc::XMLNode policystore = (*cfg)["PDPConfig"]["PolicyStore"];
  if(policystore) {
    for(int i = 0; ; i++) {
      policyfile = policystore["Location"][i];
      if(!policyfile) break;
      eval->addPolicy(SourceFile((std::string)policyfile));
      logger.msg(Arc::INFO, "Policy location: %s", (std::string)policyfile);
    }
  }
}

Charon::~Charon(void) {
  if(eval)
    delete eval;
  eval = NULL;
}

} // namespace ArcSec

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "charon", "HED:SERVICE", 0, &ArcSec::get_service },
    { NULL, NULL, 0, NULL }
};

