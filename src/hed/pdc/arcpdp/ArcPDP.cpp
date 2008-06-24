#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/loader/PDPLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>

#include "ArcPDP.h"

Arc::Logger ArcSec::ArcPDP::logger(ArcSec::PDP::logger,"ArcPDP");

/*
static ArcSec::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ArcSec::ArcPDP(cfg);
}

pdp_descriptors ARC_PDP_LOADER = {
    { "arc.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};
*/

using namespace Arc;

namespace ArcSec {

PDP* ArcPDP::get_arc_pdp(Config *cfg,ChainContext*) {
    return new ArcPDP(cfg);
}

// This class is used to store Evaluator per connection
class ArcPDPContext:public Arc::MessageContextElement {
 friend class ArcPDP;
 private:
  Evaluator* eval;
 public:
  ArcPDPContext(Evaluator* e);
  ArcPDPContext(void);
  virtual ~ArcPDPContext(void);
};

ArcPDPContext::~ArcPDPContext(void) {
  if(eval) delete eval;
}

ArcPDPContext::ArcPDPContext(Evaluator* e):eval(e) {
}

ArcPDPContext::ArcPDPContext(void):eval(NULL) {
  std::string evaluator = "arc.evaluator"; 
  EvaluatorLoader eval_loader;
  eval = eval_loader.getEvaluator(evaluator);
}

ArcPDP::ArcPDP(Config* cfg):PDP(cfg) /*, eval(NULL)*/ {
  XMLNode pdp_node(*cfg);

  XMLNode filter = (*cfg)["Filter"];
  if((bool)filter) {
    XMLNode select_attr = filter["Select"];
    XMLNode reject_attr = filter["Reject"];
    for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
    for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
  };
  XMLNode policy_store = (*cfg)["PolicyStore"];
  XMLNode policy_location = policy_store["Location"];
  for(;(bool)policy_location;++policy_location) policy_locations.push_back((std::string)policy_location);
}

bool ArcPDP::isPermitted(Message *msg){
  //Compose Request based on the information inside message, the Request will be like below:
  /*
  <Request xmlns="http://www.nordugrid.org/schemas/request-arc">
    <RequestItem>
        <Subject>
          <Attribute AttributeId="123" Type="string">123.45.67.89</Attribute>
          <Attribute AttributeId="xyz" Type="string">/O=NorduGrid/OU=UIO/CN=test</Attribute>
        </Subject>
        <Action AttributeId="ijk" Type="string">GET</Action>
    </RequestItem>
  </Request>
  */
  Evaluator* eval = NULL;

  std::string ctxid = "arcsec.arcpdp";
  try {
    Arc::MessageContextElement* mctx = (*(msg->Context()))[ctxid];
    if(mctx) {
      ArcPDPContext* pdpctx = dynamic_cast<ArcPDPContext*>(mctx);
      if(pdpctx) {
        eval=pdpctx->eval;
      };
    };
  } catch(std::exception& e) { };
  if(!eval) {
    ArcPDPContext* pdpctx = new ArcPDPContext();
    if(pdpctx) {
      eval=pdpctx->eval;
      if(eval) {
        //for(Arc::AttributeIterator it = (msg->Attributes())->getAll("PDP:POLICYLOCATION"); it.hasMore(); it++) {
        //  eval->addPolicy(SourceFile(*it));
        //}
        for(std::list<std::string>::iterator it = policy_locations.begin(); it!= policy_locations.end(); it++) {
          eval->addPolicy(SourceFile(*it));
        }
        msg->Context()->Add(ctxid, pdpctx);
      } else {
        delete pdpctx;
      }
    }
    if(!eval) logger.msg(ERROR, "Can not dynamically produce Evaluator");
  }
  if(!eval) {
    logger.msg(ERROR,"Evaluator for ArcPDP was not loaded"); 
    return false;
  };

  MessageAuth* mauth = msg->Auth()->Filter(select_attrs,reject_attrs);
  MessageAuth* cauth = msg->AuthContext()->Filter(select_attrs,reject_attrs);
  if((!mauth) && (!cauth)) {
    logger.msg(ERROR,"Missing security object in message");
    return false;
  };
  NS ns;
  XMLNode requestxml(ns,"");
  if(mauth) {
    if(!mauth->Export(SecAttr::ARCAuth,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return false;
    };
    delete mauth;
  };
  if(cauth) {
    if(!cauth->Export(SecAttr::ARCAuth,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to ARC request");
      return false;
    };
    delete cauth;
  };
  {
    std::string s;
    requestxml.GetXML(s);
    logger.msg(VERBOSE,"ARC Auth. request: %s",s);
  };
  if(requestxml.Size() <= 0) {
    logger.msg(ERROR,"No requested security information was collected");
    return false;
  };

  //Call the evaluation functionality inside Evaluator
  Response *resp = eval->evaluate(requestxml);
  logger.msg(INFO, "There are %d requests, which satisfy at least one policy", (resp->getResponseItems()).size());
  ResponseList rlist = resp->getResponseItems();
  int size = rlist.size();
  for(int i = 0; i < size; i++) {
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
        if(attrval) logger.msg(INFO, "%s", attrval->encode());
      }
    }
  } 

  //Get the number of <RequestItem/> in request (after splitting), and compare it with the 
  //number <RequestItem/> number in response. If the two value does not match, it means 
  //some <RequestItem/> in the request does not satisfy the policy. Here, we simply treat it 
  //as "Unauthorization"

  if(rlist.size() != 0){
    logger.msg(INFO, "Authorized from arc.pdp");
    if(resp)
      delete resp;
    return true;
  }
  else {
    logger.msg(ERROR, "UnAuthorized from arc.pdp");
    if(resp)
      delete resp;
    return false;
  }  

  /*
  if((rlist.size()) == (resp->getRequestSize())){
    logger.msg(INFO, "Authorized from arc.pdp");
    if(resp)
      delete resp;
    return true;
  }
  else {
    logger.msg(ERROR, "UnAuthorized from arc.pdp; Some of the RequestItem does not satisfy Policy");
    if(resp)
      delete resp;
    return false;
  }
  */
}

ArcPDP::~ArcPDP(){
  //if(eval)
  //  delete eval;
  //eval = NULL;
}

} // namespace ArcSec

