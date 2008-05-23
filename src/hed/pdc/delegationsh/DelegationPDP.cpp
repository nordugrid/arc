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
    XMLNode pdp_cfg_nd("\
      <ArcConfig\
       xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\
       xmlns:pdp=\"http://www.nordugrid.org/schemas/pdp/Config\">\
       <ModuleManager>\
       </ModuleManager>\
       <Plugins Name='arcpdc'>\
            <Plugin Name='__arc_attrfactory_modules__'>attrfactory</Plugin>\
            <Plugin Name='__arc_fnfactory_modules__'>fnfactory</Plugin>\
            <Plugin Name='__arc_algfactory_modules__'>algfactory</Plugin>\
            <Plugin Name='__arc_evaluator_modules__'>evaluator</Plugin>\
            <Plugin Name='__arc_request_modules__'>request</Plugin>\
       </Plugins>\
       <pdp:PDPConfig>\
            <pdp:AttributeFactory name='attr.factory' />\
            <pdp:CombingAlgorithmFactory name='alg.factory' />\
            <pdp:FunctionFactory name='fn.factory' />\
            <pdp:Evaluator name='arc.evaluator' />\
            <pdp:Request name='arc.request' />\
       </pdp:PDPConfig>\
      </ArcConfig>");
    //Get the lib path from environment, and put it into the configuration xml node
    std::list<std::string> plugins = ArcLocation::GetPlugins();
    for(std::list<std::string>::iterator p = plugins.begin();p!=plugins.end();++p) {
      pdp_cfg_nd["ModuleManager"].NewChild("Path")=*p;
    };

    Config modulecfg(pdp_cfg_nd);
    Arc::ClassLoader* classloader = ClassLoader::getClassLoader(&modulecfg);
    std::string evaluator = "arc.evaluator";

    //Dynamically load Evaluator object according to configure information
    eval = dynamic_cast<Evaluator*>(classloader->Instance(evaluator, (void**)(void*)&pdp_cfg_nd));
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
    ResponseList rlist = resp->getResponseItems();
    int size = rlist.size();
    bool all_policies_matched = (size > 0);
    for(int i = 0; i < size; i++) {
      ResponseItem* item = rlist[i];
      if(item->pls.size() != policies_num) all_policies_matched=false;
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

    if(all_policies_matched) {
      result=true;
    }
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

