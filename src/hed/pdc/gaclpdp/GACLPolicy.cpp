#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/loader/ClassLoader.h>

#include "GACLPolicy.h"
#include "GACLRequest.h"

Arc::Logger ArcSec::GACLPolicy::logger(Arc::Logger::rootLogger, "GACLPolicy");

Arc::LoadableClass* ArcSec::GACLPolicy::get_policy(void* arg) {
  if(arg==NULL) {
    std::cerr<<"GACLPolicy creation needs XMLNode as argument"<<std::endl;
    return NULL;
  };
  Arc::XMLNode& doc = *((Arc::XMLNode*)arg);
  if(!doc) return new ArcSec::GACLPolicy;
  ArcSec::GACLPolicy* policy = new ArcSec::GACLPolicy(doc);
  if(!(*policy)) {
    delete policy;
    return NULL;
  };
  return policy;
}

//loader_descriptors __arc_policy_modules__  = {
//    { "gacl.policy", 0, &ArcSec::GACLPolicy::get_policy },
//    { NULL, 0, NULL }
//};

using namespace Arc;
using namespace ArcSec;

GACLPolicy::GACLPolicy(void) : Policy() {
  NS ns;
  policynode.Replace(XMLNode(ns,"gacl"));
}

GACLPolicy::GACLPolicy(const XMLNode node) : Policy(node) {
  if((!node) || (node.Size() == 0)) {
    logger.msg(ERROR,"Policy is empty");
    return;
  };
  if(node.Name() != "gacl") {
    logger.msg(ERROR,"Policy is not gacl");
    return;
  };
  node.New(policynode);
}

GACLPolicy::GACLPolicy(const Source& source) : Policy(source.Get()) {
  XMLNode node = source.Get();
  if((!node) || (node.Size() == 0)) {
    logger.msg(ERROR,"Policy is empty");
    return;
  };
  if(node.Name() != "gacl") {
    logger.msg(ERROR,"Policy is not gacl");
    return;
  };
  node.New(policynode);
}

static bool CompareIdentity(XMLNode pid,XMLNode rid) {
  if(pid.Size() > 0) {
    for(int n=0;;++n) {
      XMLNode pitem = pid.Child(n);
      XMLNode ritem = rid[pitem.Name()];
      for(;(bool)ritem;++ritem) {
        if(CompareIdentity(pitem,ritem)) break;
      };
      if(!ritem) return false;
    };
    return true;
  };
  return (((std::string)pid) == ((std::string)rid));
}

static void CollectActions(XMLNode actions,std::list<std::string>& actions_list) {
  for(int n = 0;;++n) {
    XMLNode action = actions.Child(n);
    if(!action) break;
    actions_list.push_back(action.Name());
  };
}

static bool FindAction(const std::string& action,const std::list<std::string>& actions) {
  for(std::list<std::string>::const_iterator act = actions.begin();act!=actions.end();++act) {
    if((*act) == action) return true;
  };
  return false;
}

Result GACLPolicy::eval(EvaluationCtx* ctx){
  if(!ctx) return DECISION_INDETERMINATE;
  Request* req = ctx->getRequest();
  if(!req) return DECISION_INDETERMINATE;
  GACLRequest* greq = dynamic_cast<GACLRequest*>(req);
  if(!greq) return DECISION_INDETERMINATE;
  // Although it is possible to split GACL request and policy to 
  // attributes current implementation simply evaluates XMLs directly.
  // Doing it "right way" is TODO.
  //Result result = DECISION_DENY;
  XMLNode requestentry = greq->getXML();
  if(requestentry.Name() == "gacl") requestentry=requestentry["entry"];
  if(requestentry.Name() != "entry") return DECISION_INDETERMINATE;
  for(;(bool)requestentry;++requestentry) {
    XMLNode policyentry = policynode["entry"];  
    std::list<std::string> allow;
    std::list<std::string> deny;
    for(;(bool)policyentry;++policyentry) {
      bool matched = false;
      for(int n = 0;;++n) {
        XMLNode pid = policyentry.Child(n);
        if(!pid) break;
        if(pid.Name() == "allow") continue;
        if(pid.Name() == "deny") continue;
        if(pid.Name() == "any-user") { matched=true; break; };
        // TODO: somehow check if user really authenticated
        if(pid.Name() == "auth-user") { matched=true; break; };
        XMLNode rid = requestentry[pid.Name()];
        for(;(bool)rid;++rid) {
          if(CompareIdentity(pid,rid)) break;
        };
        if((bool)rid) { matched=true; break; };
      };
      if(matched) {
        XMLNode pallow = policyentry["allow"];
        XMLNode pdeny  = policyentry["deny"];
        CollectActions(pallow,allow);
        CollectActions(pdeny,deny);
      };
    };
    allow.sort(); allow.unique();
    deny.sort();  deny.unique();
    if(allow.empty()) return DECISION_DENY;
    std::list<std::string> rallow;
    CollectActions(requestentry["allow"],rallow);
    if(rallow.empty()) return DECISION_DENY; // Unlikely to happen 
    std::list<std::string>::iterator act = rallow.begin();
    for(;act!=rallow.end();++act) {
      if(!FindAction(*act,allow)) break;
      if(FindAction(*act,deny)) break;
    };
    if(act != rallow.end()) return DECISION_DENY;
    //if(act == rallow.end()) result=DECISION_PERMIT;
  };
  return DECISION_PERMIT;
  //return result;
}

EvalResult& GACLPolicy::getEvalResult(){
  return evalres;
}

void GACLPolicy::setEvalResult(EvalResult& res){
  evalres = res;
}

GACLPolicy::~GACLPolicy(){
}
