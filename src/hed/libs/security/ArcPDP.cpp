#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <arc/loader/PDPLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

#include <arc/security/ArcPDP/ArcRequest.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>
#include <arc/security/ArcPDP/Response.h>

#include "ArcPDP/attr/AttributeValue.h"

#include "ArcPDP.h"
/*
static Arc::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::ArcPDP(cfg);
}

pdp_descriptors ARC_PDP_LOADER = {
    { "arc.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};
*/
using namespace Arc;

PDP* ArcPDP::get_arc_pdp(Config *cfg,ChainContext *ctx __attribute__((unused))) {
    return new ArcPDP(cfg);
}

ArcPDP::ArcPDP(Config* cfg):PDP(cfg){
  XMLNode node(*cfg);
  XMLNode nd = node.GetRoot();
  Config topcfg(nd);

  eval = new Evaluator(topcfg);

}

bool ArcPDP::isPermitted(std::string peer_subject __attribute__((unused))){
  Response *resp = NULL;
  resp = eval->evaluate("Request.xml");
  ResponseList::iterator respit;
  logger.msg(INFO, "There is : %d subjects, which satisty at least one policy", (resp->getResponseItems()).size());
  ResponseList rlist = resp->getResponseItems();
  for(respit = rlist.begin(); respit != rlist.end(); ++respit){
    RequestTuple* tp = (*respit)->reqtp;
    Subject::iterator it;
    Arc::Subject subject = tp->sub;
    for (it = subject.begin(); it!= subject.end(); it++){
      AttributeValue *attrval;
      RequestAttribute *attr;
      attr = dynamic_cast<RequestAttribute*>(*it);
      if(attr){
        attrval = (*it)->getAttributeValue();
        if(attrval) std::cout<<attrval->encode()<<std::endl;
      }
    }
  } 

  if(!(rlist.empty())){
    logger.msg(INFO, "Authorized from arc.pdp!!!");
    if(resp)
      delete resp;
    return true;
  }
  else{
    logger.msg(ERROR, "UnAuthorized from arc.pdp!!!");
    if(resp)
      delete resp;
    return false;
  }
}

ArcPDP::~ArcPDP(){
  if(eval)
    delete eval;
  eval = NULL;
}
