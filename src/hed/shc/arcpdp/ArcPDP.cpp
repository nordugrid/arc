#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>

#include "ArcPDP.h"

Arc::Logger ArcSec::ArcPDP::logger(Arc::Logger::getRootLogger(), "ArcSec.ArcPDP");

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

Plugin* ArcPDP::get_arc_pdp(PluginArgument* arg) {
    ArcSec::PDPPluginArgument* pdparg =
            arg?dynamic_cast<ArcSec::PDPPluginArgument*>(arg):NULL;
    if(!pdparg) return NULL;
    return new ArcPDP((Arc::Config*)(*pdparg),arg);
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

ArcPDP::ArcPDP(Config* cfg,Arc::PluginArgument* parg):PDP(cfg,parg) /*, eval(NULL)*/ {
  XMLNode pdp_node(*cfg);

  XMLNode filter = (*cfg)["Filter"];
  if((bool)filter) {
    XMLNode select_attr = filter["Select"];
    XMLNode reject_attr = filter["Reject"];
    for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
    for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
  };
  XMLNode policy_store = (*cfg)["PolicyStore"];
  for(;(bool)policy_store;++policy_store) {
    XMLNode policy_location = policy_store["Location"];
    policy_locations.push_back((std::string)policy_location);
  };
  XMLNode policy = (*cfg)["Policy"];
  for(;(bool)policy;++policy) policies.AddNew(policy);
  policy_combining_alg = (std::string)((*cfg)["PolicyCombiningAlg"]);
}

PDPStatus ArcPDP::isPermitted(Message *msg) const {
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
      }
      else { logger.msg(INFO, "Can not find ArcPDPContext"); }
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
        for(std::list<std::string>::const_iterator it = policy_locations.begin(); it!= policy_locations.end(); it++) {
          eval->addPolicy(SourceFile(*it));
        }
        for(int n = 0;n<policies.Size();++n) {
          eval->addPolicy(Source(const_cast<Arc::XMLNodeContainer&>(policies)[n]));
        }
        if(!policy_combining_alg.empty()) {
          if(policy_combining_alg == "EvaluatorFailsOnDeny") {
            eval->setCombiningAlg(EvaluatorFailsOnDeny);
          } else if(policy_combining_alg == "EvaluatorStopsOnDeny") {
            eval->setCombiningAlg(EvaluatorStopsOnDeny);
          } else if(policy_combining_alg == "EvaluatorStopsOnPermit") {
            eval->setCombiningAlg(EvaluatorStopsOnPermit);
          } else if(policy_combining_alg == "EvaluatorStopsNever") {
            eval->setCombiningAlg(EvaluatorStopsNever);
          } else {
            AlgFactory* factory = eval->getAlgFactory();
            if(!factory) {
              logger.msg(WARNING, "Evaluator does not support loadable Combining Algorithms");
            } else {
              CombiningAlg* algorithm = factory->createAlg(policy_combining_alg);
              if(!algorithm) {
                logger.msg(ERROR, "Evaluator does not support specified Combining Algorithm - %s",policy_combining_alg);
              } else {
                eval->setCombiningAlg(algorithm);
              };
            };
          };
        };
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
    logger.msg(DEBUG,"ARC Auth. request: %s",s);
  };
  if(requestxml.Size() <= 0) {
    logger.msg(ERROR,"No requested security information was collected");
    return false;
  };

  //Call the evaluation functionality inside Evaluator
  Response *resp = eval->evaluate(requestxml);
  if(!resp) {
    logger.msg(ERROR, "Not authorized by arc.pdp - failed to get response from Evaluator");
    return false;
  };
  ResponseList rlist = resp->getResponseItems();
  int size = rlist.size();

  //  The current ArcPDP is supposed to be used as policy decision point for Arc1 HED components, and
  // those services which are based on HED.
  //  Each message/session comes with one unique <Subject/> (with a number of <Attribute/>s),
  // and different <Resource/> and <Action/> elements (possibly plus <Context/>).
  //  The results from all tuples are combined using following decision algorithm: 
  // 1. If any of tuples made of <Subject/>, <Resource/>, <Action/> and <Context/> gets "DENY"
  //    then final result is negative (false).
  // 2. Otherwise if any of tuples gets "PERMIT" then final result is positive (true).
  // 3. Otherwise result is negative (false).

  bool atleast_onedeny = false;
  bool atleast_onepermit = false;

  for(int i = 0; i < size; i++) {
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
        if(attrval) logger.msg(DEBUG, "%s", attrval->encode());
      }
    }
  } 
  
  bool result = false;
  if(atleast_onedeny) result = false;
  else if(atleast_onepermit) result = true;
  else result = false;

  if(result) logger.msg(VERBOSE, "Authorized by arc.pdp");
  else logger.msg(INFO, "Not authorized by arc.pdp - some of the RequestItem elements do not satisfy Policy");
  
  if(resp) delete resp;
    
  return result;
}

ArcPDP::~ArcPDP(){
  //if(eval)
  //  delete eval;
  //eval = NULL;
}

} // namespace ArcSec

