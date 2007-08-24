#include "ArcPolicy.h"
#include <list>
#include "../Result.h"

ArcPolicy::ArcPolicy(const XMLNode& node){
  ArcRule *rule;
  XMLNode nd;
  ArcAlgFactory *algfactory = new ArcAlgFactory(); 
  
  id = (std::string)(node.Attribute("PolicyID"));

  //Setup the rules combining algorithm inside one policy
  if(node.Attribute("CombiningAlg") != NULL)
    comalg = algfactory->createCombiningAlg((std::string)(node.Attribute("CombiningAlg")));
  else comalg = algfactory->createCombiningAlg("deny-overides");     

  description = (std::string)(node["Description"]);
  
  for ( int i=0;; i++ ){
    nd = node["Rule"][i];
    if(!nd) break;
    rule = new ArcRule(nd);
    subelements.push_back(rule);
  }
    
}

MatchResult ArcPolicy::match(const EvaluationCtx* ctx){
  //Arc::RequestTuple evaltuple = ctx->getEvalTuple();
  
  //Because ArcPolicy definition has no any <Subject, Resource, Action, Environment> directly;
  //All the <Subject, Resource, Action, Environment>s are only in ArcRule.
  //So the function always return "Match" 

  return Match;
  
}

Result ArcPolicy::eval(const EvaluationCtx* ctx){
//  Arc::RequestTuple evaltuple = ctx->getEvalTuple();
  
  Result result = comalg->combine(ctx, subelements);

  return result;
   
}

ArcPolicy::~ArcPolicy(){
  while(!subelements.empty()){
      delete subelements.back();
      subelements.pop_back();
  }

}
