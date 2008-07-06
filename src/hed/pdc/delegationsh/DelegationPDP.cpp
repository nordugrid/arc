#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <arc/loader/PDPLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>

#include "DelegationPDP.h"

// NOTE: using factories/attrbites provided ArcPDP
//  This PDP is mostly same as ArcPDP. The difference 
//  is that it takes both request and policies from SecAttr.
//  Also currently for flexibility it performs every evaluation
//  per request. Later it should become clever and distinguish
//  if delegated policy comes per request or per session.

Arc::Logger ArcSec::DelegationPDP::logger(ArcSec::PDP::logger,"DelegationPDP");

using namespace Arc;

namespace ArcSec {

PDP* DelegationPDP::get_delegation_pdp(Config *cfg,ChainContext*) {
    return new DelegationPDP(cfg);
}

DelegationPDP::DelegationPDP(Config* cfg):PDP(cfg) {
  XMLNode pdp_node(*cfg);
  XMLNode filter = (*cfg)["Filter"];
  if((bool)filter) {
    XMLNode select_attr = filter["Select"];
    XMLNode reject_attr = filter["Reject"];
    for(;(bool)select_attr;++select_attr) select_attrs.push_back((std::string)select_attr);
    for(;(bool)reject_attr;++reject_attr) reject_attrs.push_back((std::string)reject_attr);
  };
}

DelegationPDP::~DelegationPDP(){
}

bool DelegationPDP::isPermitted(Message *msg){
  MessageAuth* mauth = msg->Auth()->Filter(select_attrs,reject_attrs);
  MessageAuth* cauth = msg->AuthContext()->Filter(select_attrs,reject_attrs);
  if((!mauth) && (!cauth)) {
    logger.msg(ERROR,"Missing security object in message");
    return false;
  };

  // Extract policies
  // TODO: Probably make MessageAuth do it or there should be some other way
  //       to avoid multiple extraction of same object.
  // Currently delegated policies are simply stored under special name "DELEGATION POLICY"
  // To have multiple policies in same object MultiSecAttr class may be used. Then
  // Policies are catenated under top-level element "Policies".
  // Otherwise in case of single policy (current implementation) top-level element is "Policy".

  bool result = false;
  Evaluator* eval = NULL;
  try {
    SecAttr* mpolicy_attr = mauth?(*mauth)["DELEGATION POLICY"]:NULL;
    SecAttr* cpolicy_attr = cauth?(*cauth)["DELEGATION POLICY"]:NULL;
    if((cpolicy_attr == NULL) && (mpolicy_attr == NULL)) {
      logger.msg(INFO,"No delegation policies in this context and message - passing through");
      result=true; throw std::exception();
    };

    // Create evaluator
    std::string evaluator = "arc.evaluator";
    EvaluatorLoader eval_loader;
    eval = eval_loader.getEvaluator(evaluator);
    if(!eval) {
      logger.msg(ERROR, "Can not dynamically produce Evaluator");
      throw std::exception();
    };
    // Just make sure algorithm is proper one
    eval->setCombiningAlg(EvaluatorFailsOnDeny);

    // Add policies to evaluator
    int policies_num = 0;
    if(mpolicy_attr) {
      NS ns; XMLNode policyxml(ns,"");
      if(!mpolicy_attr->Export(SecAttr::ARCAuth,policyxml)) {
        logger.msg(ERROR,"Failed to convert security information to ARC policy");
        throw std::exception();
      };
      if(policyxml.Name() == "Policy") {
        eval->addPolicy(policyxml); ++policies_num;
      } else if(policyxml.Name() == "Policies") {
        for(XMLNode p = policyxml["Policy"];(bool)p;++p) {
          eval->addPolicy(p); ++policies_num;
        };
      };
    };
    if(cpolicy_attr) {
      NS ns; XMLNode policyxml(ns,"");
      if(!cpolicy_attr->Export(SecAttr::ARCAuth,policyxml)) {
        logger.msg(ERROR,"Failed to convert security information to ARC policy");
        throw std::exception();
      };
      if(policyxml.Name() == "Policy") {
        eval->addPolicy(policyxml); ++policies_num;
        {
          std::string s; policyxml.GetXML(s);
          logger.msg(VERBOSE,"ARC delegation policy: %s",s);
        };
      } else if(policyxml.Name() == "Policies") {
        for(XMLNode p = policyxml["Policy"];(bool)p;++p) {
          eval->addPolicy(p); ++policies_num;
          {
            std::string s; policyxml.GetXML(s);
            logger.msg(VERBOSE,"ARC delegation policy: %s",s);
          };
        };
      };
    };
    if(policies_num == 0) {
      logger.msg(INFO,"No delegation policies in this context and message - passing through");
      result=true; throw std::exception();
    };

    // Generate request
    NS ns; XMLNode requestxml(ns,"");
    if(mauth) {
      if(!mauth->Export(SecAttr::ARCAuth,requestxml)) {
        logger.msg(ERROR,"Failed to convert security information to ARC request");
        throw std::exception();
      };
    };
    if(cauth) {
      if(!cauth->Export(SecAttr::ARCAuth,requestxml)) {
        logger.msg(ERROR,"Failed to convert security information to ARC request");
        throw std::exception();
      };
    };
    {
      std::string s;
      requestxml.GetXML(s);
      logger.msg(VERBOSE,"ARC Auth. request: %s",s);
    };
    if(requestxml.Size() <= 0) {
      logger.msg(ERROR,"No requested security information was collected");
      throw std::exception();
    };

   
    //Call the evaluation functionality inside Evaluator
    Response *resp = eval->evaluate(requestxml);
    logger.msg(INFO, "There are %d requests, which satisfy at least one policy", (resp->getResponseItems()).size());
    bool atleast_onedeny = false;
    bool atleast_onepermit = false;
    if(!resp) {
      logger.msg(ERROR,"No authorization response was returned");
      throw std::exception();
    };

    ResponseList rlist = resp->getResponseItems();
    int size = rlist.size();
    for(int i = 0; i < size; i++) {
      ResponseItem* item = rlist[i];
      RequestTuple* tp = item->reqtp;
      if(item->res == DECISION_DENY) atleast_onedeny = true;
      if(item->res == DECISION_PERMIT) atleast_onepermit = true;
    }
    delete resp;

    if(atleast_onepermit) result = true;
    if(atleast_onedeny) result = false;

  } catch(std::exception&) {
  };
  if(result) {
    logger.msg(INFO, "Delegation authorization passed");
  } else {
    logger.msg(INFO, "Delegation authorization failed");
  };
  if(mauth) delete mauth;
  if(cauth) delete cauth;
  if(eval) delete eval;
  return result;
}


} // namespace ArcSec

