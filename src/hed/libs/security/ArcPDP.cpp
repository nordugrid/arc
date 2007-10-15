#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <arc/loader/PDPLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

#include <arc/security/ArcPDP/Response.h>

#include <arc/security/ArcPDP/attr/AttributeValue.h>

#include "ArcPDP.h"
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
using namespace ArcSec;

PDP* ArcPDP::get_arc_pdp(Config *cfg,ChainContext*) {
    return new ArcPDP(cfg);
}

ArcPDP::ArcPDP(Config* cfg):PDP(cfg){
  XMLNode node(*cfg);
  XMLNode nd = node.GetRoot();
  Config topcfg(nd);

  eval = new ArcEvaluator(topcfg);

}

bool ArcPDP::isPermitted(Message *msg){
  std::string remotehost=msg->Attributes()->get("TCP:REMOTEHOST");
  std::string subject=msg->Attributes()->get("TLS:PEERDN");
  std::string action=msg->Attributes()->get("HTTP:METHOD");
  
  NS ns;
  ns["ra"]="http://www.nordugrid.org/ws/schemas/request-arc";
  XMLNode request(ns,"ra:Request");
  XMLNode requestitem = request.NewChild("ra:RequestItem");

  XMLNode sub = requestitem.NewChild("ra:Subject");
  XMLNode subattr1 = sub.NewChild("ra:Attribute");
  subattr1 = remotehost;
  XMLNode subattr1Id = subattr1.NewAttribute("ra:AttributeId");
  subattr1Id = "123";
  XMLNode subattr1Type = subattr1.NewAttribute("ra:Type");
  subattr1Type = "string";

  XMLNode subattr2 = sub.NewChild("ra:Attribute");
  subattr2 = subject;
  XMLNode subattr2Id = subattr2.NewAttribute("ra:AttributeId");
  subattr2Id = "xyz";
  XMLNode subattr2Type = subattr2.NewAttribute("ra:Type");
  subattr2Type = "string";

  XMLNode act = requestitem.NewChild("ra:Action");
  act=action;
  XMLNode actionId = act.NewAttribute("ra:AttributeId");
  actionId = "ijk";
  XMLNode actionType = act.NewAttribute("ra:Type");
  actionType = "string";

  std::string req_str;  
  request.GetDoc(req_str);
  logger.msg(INFO, "%s", req_str.c_str());

  Response *resp = NULL;
  //resp = eval->evaluate("Request.xml");
  resp = eval->evaluate(request);
  logger.msg(INFO, "There is : %d subjects, which satisty at least one policy", (resp->getResponseItems()).size());
  ResponseList rlist = resp->getResponseItems();
  int size = rlist.size();
  for(int i = 0; i < size; i++){
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
        if(attrval) logger.msg(INFO, "%s", (attrval->encode()).c_str());
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
