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

#include "XACMLPDP.h"

Arc::Logger ArcSec::XACMLPDP::logger(Arc::Logger::getRootLogger(), "ArcSec.XACMLPDP");

using namespace Arc;

namespace ArcSec {

Plugin* XACMLPDP::get_xacml_pdp(PluginArgument* arg) {
    ArcSec::PDPPluginArgument* pdparg =
            arg?dynamic_cast<ArcSec::PDPPluginArgument*>(arg):NULL;
    if(!pdparg) return NULL;
    return new XACMLPDP((Arc::Config*)(*pdparg));
}

// This class is used to store Evaluator per connection
class XACMLPDPContext:public Arc::MessageContextElement {
 friend class XACMLPDP;
 private:
  Evaluator* eval;
 public:
  XACMLPDPContext(Evaluator* e);
  XACMLPDPContext(void);
  virtual ~XACMLPDPContext(void);
};

XACMLPDPContext::~XACMLPDPContext(void) {
  if(eval) delete eval;
}

XACMLPDPContext::XACMLPDPContext(Evaluator* e):eval(e) {
}

XACMLPDPContext::XACMLPDPContext(void):eval(NULL) {
  std::string evaluator = "xacml.evaluator"; 
  EvaluatorLoader eval_loader;
  eval = eval_loader.getEvaluator(evaluator);
}

XACMLPDP::XACMLPDP(Config* cfg):PDP(cfg) /*, eval(NULL)*/ {
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
  XMLNode policy = (*cfg)["Policy"];
  for(;(bool)policy;++policy) policies.AddNew(policy);
  policy_combining_alg = (std::string)((*cfg)["PolicyCombiningAlg"]);
}

bool XACMLPDP::isPermitted(Message *msg) const {
  //Compose Request based on the information inside message, the Request will be
  //compatible to xacml request schema

  Evaluator* eval = NULL;

  const std::string ctxid = "arcsec.xacmlpdp";
  try {
    Arc::MessageContextElement* mctx = (*(msg->Context()))[ctxid];
    if(mctx) {
      XACMLPDPContext* pdpctx = dynamic_cast<XACMLPDPContext*>(mctx);
      if(pdpctx) {
        eval=pdpctx->eval;
      }
      else { logger.msg(INFO, "Can not find XACMLPDPContext"); }
    };
  } catch(std::exception& e) { };
  if(!eval) {
    XACMLPDPContext* pdpctx = new XACMLPDPContext();
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
    logger.msg(ERROR,"Evaluator for XACMLPDP was not loaded"); 
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
    if(!mauth->Export(SecAttr::XACML,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to XACML request");
      return false;
    };
    delete mauth;
  };
  if(cauth) {
    if(!cauth->Export(SecAttr::XACML,requestxml)) {
      delete mauth;
      logger.msg(ERROR,"Failed to convert security information to XACML request");
      return false;
    };
    delete cauth;
  };
  {
    std::string s;
    requestxml.GetXML(s);
    logger.msg(DEBUG,"XACML request: %s",s);
  };
  if(requestxml.Size() <= 0) {
    logger.msg(ERROR,"No requested security information was collected");
    return false;
  };

  //Call the evaluation functionality inside Evaluator
  Response *resp = eval->evaluate(requestxml);
  ArcSec::ResponseList rlist = resp->getResponseItems();
  std::cout<<rlist[0]->res<<std::endl;
  bool result = false;
  if(rlist[0]->res == DECISION_PERMIT) { logger.msg(INFO, "Authorized from xacml.pdp"); result = true; }
  else logger.msg(ERROR, "UnAuthorized from xacml.pdp");
  
  if(resp) delete resp;
    
  return result;
}

XACMLPDP::~XACMLPDP(){
}

} // namespace ArcSec

