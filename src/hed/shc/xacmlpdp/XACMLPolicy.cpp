#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>

#include "XACMLPolicy.h"
#include "XACMLRule.h"

Arc::Logger ArcSec::XACMLPolicy::logger(Arc::Logger::rootLogger, "XACMLPolicy");

static Arc::NS policyns("policy", "urn:oasis:names:tc:xacml:2.0:policy:schema:os");

/** get_policy (in charge of class-loading of XACMLPolicy) can only
 * accept one type of argument--XMLNode */
Arc::Plugin* ArcSec::XACMLPolicy::get_policy(Arc::PluginArgument* arg) {
    //std::cout<<"Argument type of XACMLPolicy:"<<typeid(arg).name()<<std::endl;
    if(arg==NULL) return NULL;
    Arc::ClassLoaderPluginArgument* clarg =
            arg?dynamic_cast<Arc::ClassLoaderPluginArgument*>(arg):NULL;
    if(!clarg) return NULL;
    // Check if empty or valid policy is supplied
    Arc::XMLNode* doc = (Arc::XMLNode*)(*clarg);
    if(doc==NULL) {
        std::cerr<<"XACMLPolicy creation requires XMLNode as argument"<<std::endl;
        return NULL;
    }
    //if(!(*doc)) return new ArcSec::XACMLPolicy;
    ArcSec::XACMLPolicy* policy = new ArcSec::XACMLPolicy(*doc);
    if((!policy) || (!(*policy))) {
      delete policy;
      return NULL;
    };
    return policy;
}

using namespace Arc;
using namespace ArcSec;

XACMLPolicy::XACMLPolicy(void) : Policy(), comalg(NULL) {
  Arc::XMLNode newpolicy(policyns,"policy:Policy");
  newpolicy.New(policynode);
  policytop=policynode;
}

XACMLPolicy::XACMLPolicy(const XMLNode node) : Policy(node), comalg(NULL), target(NULL) {
  if((!node) || (node.Size() == 0)) {
    logger.msg(ERROR,"Policy is empty");
    return;
  }
  node.New(policynode);

  std::list<XMLNode> res = policynode.XPathLookup("//policy:Policy",policyns);
  if(res.empty()) {
    logger.msg(ERROR,"Can not find <Policy/> element with proper namespace");
    policynode.Destroy();
    return;
  }
  policytop = *(res.begin());
}

XACMLPolicy::XACMLPolicy(const XMLNode node, EvaluatorContext* ctx) : Policy(node), comalg(NULL), target(NULL) {
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
  setEvaluatorContext(ctx);
  make_policy();
}

void XACMLPolicy::make_policy() {
  //EvalResult.node record the policy(in XMLNode) information about evaluation result.
  //According to the developer's requirement, 
  //EvalResult.node can include rules(in XMLNode) that "Permit" or "Deny" 
  //the request tuple. In the existing code, it include all 
  //the original rules.

  if(!policynode) return;
  if(!policytop) return;

  evalres.node = policynode;
  evalres.effect = "Not_applicable";

  XACMLRule *rule;
  //Get AlgFactory from EvaluatorContext
  algfactory = (AlgFactory*)(*evaluatorctx); 

  XMLNode nd = policytop;
  XMLNode rnd;
  if((bool)nd){
    nd = policytop;
    id = (std::string)(nd.Attribute("PolicyId"));

    //Setup the rules' combining algorithm inside one policy, according to the "RuleCombiningAlgId" name
    if(nd.Attribute("RuleCombiningAlgId")) {
      std::string tmpstr = (std::string)(nd.Attribute("RuleCombiningAlgId"));
      size_t found = tmpstr.find_last_of(":");
      std::string algstr = tmpstr.substr(found+1);
      if(algstr == "deny-overrides") algstr = "Deny-Overrides";
      else if(algstr == "permit-overrides") algstr = "Permit-Overrides";
      comalg = algfactory->createAlg(algstr);
    }
    else comalg = algfactory->createAlg("Deny-Overrides");
    
    description = (std::string)(nd["Description"]);  
  }
  
  logger.msg(INFO, "PolicyId: %s  Alg inside this policy is:-- %s", id, comalg?(comalg->getalgId()):"");

  XMLNode targetnode = nd["Target"];
  if(((bool)targetnode) && ((bool)(targetnode.Child()))) 
    target = new XACMLTarget(targetnode, evaluatorctx);

  for ( int i=0;; i++ ){
    rnd = nd["Rule"][i];
    if(!rnd) break;
    rule = new XACMLRule(rnd, evaluatorctx);
    subelements.push_back(rule);
  }
}

MatchResult XACMLPolicy::match(EvaluationCtx* ctx){
  MatchResult res;
  if(target != NULL) res = target->match(ctx);
  else { logger.msg(Arc::INFO, "No target available inside the policy"); res = INDETERMINATE; }
  return res;
}

Result XACMLPolicy::eval(EvaluationCtx* ctx){
  Result result = DECISION_NOT_APPLICABLE;
  MatchResult matchres = match(ctx);
  if(matchres == NO_MATCH)  return result;
  else if(matchres == INDETERMINATE) {result = DECISION_INDETERMINATE; return result;}
 
  result = comalg?comalg->combine(ctx, subelements):DECISION_INDETERMINATE;
  if(result == DECISION_PERMIT) evalres.effect = "Permit";
  else if(result == DECISION_DENY) evalres.effect = "Deny";
  else if(result == DECISION_INDETERMINATE) evalres.effect = "Indeterminate";

  return result;
}

EvalResult& XACMLPolicy::getEvalResult(){
  return evalres;
}

XACMLPolicy::~XACMLPolicy(){
  while(!(subelements.empty())){
    delete subelements.back();
    subelements.pop_back();
  }
  if(target != NULL) delete target;
}
