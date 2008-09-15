#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/ClassLoader.h>

#include "GACLPolicy.h"
#include "GACLRequest.h"

#include "GACLEvaluator.h"

static Arc::LoadableClass* get_evaluator(void* arg) {
    return new ArcSec::GACLEvaluator((Arc::XMLNode*) arg);
}

loader_descriptors __arc_evaluator_modules__  = {
    { "gacl.evaluator", 0, &get_evaluator },
    { NULL, 0, NULL }
};

using namespace Arc;
using namespace ArcSec;

Arc::Logger ArcSec::GACLEvaluator::logger(Arc::Logger::rootLogger, "GACLEvaluator");

GACLEvaluator::GACLEvaluator(Arc::XMLNode* cfg) : Evaluator(cfg) /* , m_cfg(cfg) */ {
  plstore = NULL;;
  plstore = new PolicyStore("", "gacl.policy", NULL);
  if(!plstore) logger.msg(ERROR, "Can not create PolicyStore object");
  combining_alg = EvaluatorFailsOnDeny;
}

GACLEvaluator::GACLEvaluator(const char * cfgfile) : Evaluator(cfgfile){
  plstore = new PolicyStore("", "gacl.policy", NULL);
  if(!plstore) logger.msg(ERROR, "Can not create PolicyStore object");
  combining_alg = EvaluatorFailsOnDeny;
}

Response* GACLEvaluator::evaluate(Request* request, Policy* policyobj) {
  GACLPolicy* gpol = dynamic_cast<GACLPolicy*>(policyobj);
  if(!gpol) return NULL;
  GACLRequest* greq = dynamic_cast<GACLRequest*>(request);
  if(!greq) return NULL;
  EvaluationCtx ctx(greq);
  ResponseItem* ritem = new ResponseItem;
  if(!ritem) return NULL;
  Response* resp = new Response();
  if(!resp) { delete ritem; return NULL; };
  resp->setRequestSize(0);
  ritem->reqtp = NULL;
  ritem->res = gpol->eval(&ctx);
  //greq->getXML().New(ritem->reqxml);
  //ritem->plsxml.push_back(gpol->getXML());
  //ritem->pls.push_back(gpol);
  resp->addResponseItem(ritem);
  return resp;
}

Response* GACLEvaluator::evaluate(const Source& request, const Source& policy) {
  GACLRequest greq(request);
  GACLPolicy gpol(policy);
  return evaluate(&greq,&gpol);
}

Response* GACLEvaluator::evaluate(Request* request, const Source& policy) {
  GACLPolicy gpol(policy);
  return evaluate(request,&gpol);
}

Response* GACLEvaluator::evaluate(const Source& request, Policy* policyobj) {
  GACLRequest greq(request);
  return evaluate(&greq,policyobj);
}

Response* GACLEvaluator::evaluate(Request* request) {
  if(!plstore) return NULL;
  GACLRequest* greq = dynamic_cast<GACLRequest*>(request);
  if(!greq) return NULL;
  EvaluationCtx ctx(greq);
  ResponseItem* ritem = new ResponseItem;
  if(!ritem) return NULL;
  Response* resp = new Response();
  if(!resp) { delete ritem; return NULL; };
  Result result = DECISION_DENY;
  std::list<PolicyStore::PolicyElement> policies = plstore->findPolicy(&ctx);
  std::list<PolicyStore::PolicyElement>::iterator policyit;
  bool have_permit = false;
  bool have_deny = false;
  bool have_indeterminate = false;
  bool have_notapplicable = false;
  for(policyit = policies.begin(); policyit != policies.end(); policyit++){
    Result res = ((Policy*)(*policyit))->eval(&ctx);
    if(res == DECISION_PERMIT){
      have_permit=true;
      if(combining_alg == EvaluatorStopsOnPermit) break;
    } else if(res == DECISION_DENY) {
      have_deny=true;
      if(combining_alg == EvaluatorStopsOnDeny) break;
      if(combining_alg == EvaluatorFailsOnDeny) break;
    } else if(res == DECISION_INDETERMINATE) {
      have_indeterminate=true;
    } else if(res == DECISION_NOT_APPLICABLE) {
      have_notapplicable=true;
    };
  };
  if(have_permit) { result = DECISION_PERMIT; }
  else if(have_deny) { result = DECISION_DENY; }
  else if(have_indeterminate) { result = DECISION_INDETERMINATE; }
  else if(have_notapplicable) { result = DECISION_NOT_APPLICABLE; };
  resp->setRequestSize(0);
  ritem->reqtp = NULL;
  ritem->res = result;
  //greq->getXML().New(ritem->reqxml);
  //ritem->plsxml.push_back(gpol->getXML());
  //ritem->pls.push_back(gpol);
  resp->addResponseItem(ritem);
  return resp;
}

Response* GACLEvaluator::evaluate(const Source& request) {
  GACLRequest greq(request);
  return evaluate(&greq);
}

GACLEvaluator::~GACLEvaluator(){
  if(plstore) delete plstore;
}

