#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "XACMLPolicy.h"
#include "XACMLRule.h"
#include <list>

Arc::Logger ArcSec::XACMLPolicy::logger(Arc::Logger::rootLogger, "XACMLPolicy");

using namespace Arc;
using namespace ArcSec;

XACMLPolicy::XACMLPolicy(XMLNode& node, EvaluatorContext* ctx) : Policy(node), comalg(NULL), target(NULL) {
  node.New(policynode);

  std::string xml;
  policynode.GetDoc(xml);
  std::cout<<xml<<std::endl;
  
  //EvalResult.node record the policy(in XMLNode) information about evaluation result. According to the developer's requirement, 
  //EvalResult.node can include rules(in XMLNode) that "Permit" or "Deny" the request tuple. In the existing code, it include all 
  //the original rules.
  evalres.node = policynode;
  evalres.effect = "Not_applicable";

  XACMLRule *rule;
  //Get AlgFactory from EvaluatorContext
  algfactory = (AlgFactory*)(*ctx); 

  XMLNode nd, rnd;

  Arc::NS nsList;
  std::list<XMLNode> res;
  nsList.insert(std::pair<std::string, std::string>("policy","http://www.nordugrid.org/ws/schemas/policy-arc"));

  res = policynode.XPathLookup("//policy:Policy", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
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

  XMLNode targetnode = node["Target"];
  if((bool)targetnode) target = new XACMLTarget(targetnode, ctx);
 
  for ( int i=0;; i++ ){
    rnd = nd["Rule"][i];
    if(!rnd) break;
    rule = new XACMLRule(rnd, ctx);
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
  Result result = comalg?comalg->combine(ctx, subelements):DECISION_INDETERMINATE;
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
