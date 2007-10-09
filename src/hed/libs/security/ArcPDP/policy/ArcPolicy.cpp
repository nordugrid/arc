#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ArcPolicy.h"
#include "ArcRule.h"
#include <list>

using namespace Arc;
using namespace ArcSec;

Logger ArcPolicy::logger(Logger::rootLogger,"ArcPolicy");

ArcPolicy::ArcPolicy(XMLNode& node, EvaluatorContext* ctx) : Policy(node), comalg(NULL) {
  node.New(policynode);
  
  //EvalResult.node record the policy(in XMLNode) information about evaluation result. According to the developer's requirement, EvalResult.node can include rule(in XMLNode) that "Permit" or "Deny" the request tuple. In the existing code, it include all the original rules.
  evalres.node = policynode;
  evalres.effect = "Not_applicable";

  ArcRule *rule;
  //ArcAlgFactory *algfactory = new ArcAlgFactory(); 
  algfactory = (AlgFactory*)(*ctx); 

  XMLNode nd, rnd;

  Arc::NS nsList;
  std::list<XMLNode> res;
  nsList.insert(std::pair<std::string, std::string>("policy","http://www.nordugrid.org/ws/schemas/policy-arc"));

  res = policynode.XPathLookup("//policy:Policy", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    id = (std::string)(nd.Attribute("PolicyId"));

    //Setup the rules combining algorithm inside one policy
    if(nd.Attribute("CombiningAlg"))
      comalg = algfactory->createAlg((std::string)(nd.Attribute("CombiningAlg")));
    else comalg = algfactory->createAlg("Deny-Overrides");
    
    description = (std::string)(nd["Description"]);  
  }
  
  logger.msg(INFO, "PolicyId: %s  Alg inside this policy is:-- %s", id.c_str(), (comalg->getalgId()).c_str());
 
  for ( int i=0;; i++ ){
    rnd = nd["Rule"][i];
    if(!rnd) break;
    rule = new ArcRule(rnd, ctx);
    subelements.push_back(rule);
  }
}

MatchResult ArcPolicy::match(EvaluationCtx*){// ctx){
  //RequestTuple* evaltuple = ctx->getEvalTuple();
  
  //Because ArcPolicy definition has no any <Subject, Resource, Action, Environment> directly;
  //All the <Subject, Resource, Action, Environment>s are only in ArcRule.
  //So the function always return "Match" 

  return MATCH;
  
}

Result ArcPolicy::eval(EvaluationCtx* ctx){
  Result result = comalg->combine(ctx, subelements);
  if(result == DECISION_PERMIT) evalres.effect = "Permit";
  else if(result == DECISION_DENY) evalres.effect = "Deny";
  else if(result == DECISION_INDETERMINATE) evalres.effect = "Indeterminate";

  return result;
}

EvalResult& ArcPolicy::getEvalResult(){
  return evalres;
}

ArcPolicy::~ArcPolicy(){
  while(!(subelements.empty())){
      delete subelements.back();
      subelements.pop_back();
  }
}
