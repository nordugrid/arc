#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include <arc/security/ClassLoader.h>

#include "ArcPolicy.h"
#include "ArcRule.h"

Arc::Logger ArcSec::ArcPolicy::logger(Arc::Logger::rootLogger, "ArcPolicy");

static Arc::NS policyns("policy", "http://www.nordugrid.org/schemas/policy-arc");

/** get_policy (in charge of class-loading of ArcPolicy) can only 
accept one type of argument--XMLNode */
Arc::Plugin* ArcSec::ArcPolicy::get_policy(Arc::PluginArgument* arg) {
    //std::cout<<"Argument type of ArcPolicy:"<<typeid(arg).name()<<std::endl;
    if(arg==NULL) return NULL;
    Arc::ClassLoaderPluginArgument* clarg =
            arg?dynamic_cast<Arc::ClassLoaderPluginArgument*>(arg):NULL;
    if(!clarg) return NULL;
    // Check if empty or valid policy is supplied
    Arc::XMLNode* doc = (Arc::XMLNode*)(*clarg);
    if(doc==NULL) { 
        std::cerr<<"ArcPolicy creation requires XMLNode as argument"<<std::endl;
        return NULL;
    }
    // NOTE: Following line is not good for autodetection. Should it be removed?
    if(!(*doc)) return new ArcSec::ArcPolicy;
    ArcSec::ArcPolicy* policy = new ArcSec::ArcPolicy(*doc);
    if((!policy) || (!(*policy))) {
      delete policy;
      return NULL;
    };
    return policy;
}

//loader_descriptors __arc_policy_modules__  = {
//    { "arc.policy", 0, &ArcSec::ArcPolicy::get_policy },
//    { NULL, 0, NULL }
//};

using namespace Arc;
using namespace ArcSec;

ArcPolicy::ArcPolicy(void) : Policy(), comalg(NULL) {
  Arc::XMLNode newpolicy(policyns,"policy:Policy");
  newpolicy.New(policynode);
  policytop=policynode;
}

ArcPolicy::ArcPolicy(const XMLNode node) : Policy(node), comalg(NULL) {
  if((!node) || (node.Size() == 0)) {
    logger.msg(ERROR,"Policy is empty");
    return;
  }
  node.New(policynode);
  std::list<XMLNode> res = policynode.XPathLookup("//policy:Policy",policyns);
  if(res.empty()) {
    policynode.Destroy();
    return;
  }
  policytop = *(res.begin());
}

ArcPolicy::ArcPolicy(const XMLNode node, EvaluatorContext* ctx) : Policy(node), comalg(NULL) {
  if((!node) || (node.Size() == 0)) {
    logger.msg(WARNING,"Policy is empty");
    return;
  }
  node.New(policynode);
  std::list<XMLNode> res = policynode.XPathLookup("//policy:Policy",policyns);
  if(res.empty()) {
    policynode.Destroy();
    return;
  }
  setEvaluatorContext(ctx); 
  make_policy();
}

void ArcPolicy::make_policy() {  
  //EvalResult.node record the policy(in XMLNode) information about evaluation result. 
  //According to the developer's requirement, EvalResult.node can include rules(in XMLNode) 
  //that "Permit" or "Deny" the request tuple. In the existing code, it include all 
  //the original rules.

  if(!policynode) return;
  if(!policytop) return;

  evalres.node = policynode;
  evalres.effect = "Not_applicable";

  ArcRule *rule;
  //Get AlgFactory from EvaluatorContext
  algfactory = (AlgFactory*)(*evaluatorctx); 

  XMLNode nd = policytop;
  XMLNode rnd;
  if((bool)nd){
    nd = policytop;
    id = (std::string)(nd.Attribute("PolicyId"));

    //Setup the rules' combining algorithm inside one policy, according to the "CombiningAlg" name
    if(nd.Attribute("CombiningAlg"))
      comalg = algfactory->createAlg((std::string)(nd.Attribute("CombiningAlg")));
    else comalg = algfactory->createAlg("Deny-Overrides");
    
    description = (std::string)(nd["Description"]);  
  }
  
  logger.msg(INFO, "PolicyId: %s  Alg inside this policy is:-- %s", id, comalg?(comalg->getalgId()):"");
 
  for ( int i=0;; i++ ){
    rnd = nd["Rule"][i];
    if(!rnd) break;
    rule = new ArcRule(rnd, evaluatorctx);
    subelements.push_back(rule);
  }
}

MatchResult ArcPolicy::match(EvaluationCtx*){// ctx){
  //RequestTuple* evaltuple = ctx->getEvalTuple();
  
  //Because ArcPolicy definition has no any <Subject, Resource, Action, Condition> directly;
  //All the <Subject, Resource, Action, Condition>s are only in ArcRule.
  //So the function always return "Match" 

  return MATCH;
  
}

Result ArcPolicy::eval(EvaluationCtx* ctx){
  Result result = comalg?comalg->combine(ctx, subelements):DECISION_INDETERMINATE;
  if(result == DECISION_PERMIT) evalres.effect = "Permit";
  else if(result == DECISION_DENY) evalres.effect = "Deny";
  else if(result == DECISION_INDETERMINATE) evalres.effect = "Indeterminate";
  else if(result == DECISION_NOT_APPLICABLE) evalres.effect = "Not_Applicable";

  return result;
}

EvalResult& ArcPolicy::getEvalResult() {
  return evalres;
}

void ArcPolicy::setEvalResult(EvalResult& res){
  evalres = res;
}

const char* ArcPolicy::getEvalName() const{
  return "arc.evaluator";
}

const char* ArcPolicy::getName() const{
  return "arc.policy";
}

ArcPolicy::~ArcPolicy(){
  while(!(subelements.empty())){
      delete subelements.back();
      subelements.pop_back();
  }
}

