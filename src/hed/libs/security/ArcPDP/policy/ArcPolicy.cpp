#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ArcPolicy.h"
#include "ArcRule.h"
#include <list>
#include "../alg/ArcAlgFactory.h"

using namespace Arc;

ArcPolicy::ArcPolicy(XMLNode& node) : Policy(node) {
  ArcRule *rule;
  ArcAlgFactory *algfactory = new ArcAlgFactory(); 
 
  XMLNode nd, rnd;

  Arc::NS nsList;
  std::list<XMLNode> res;
  nsList.insert(std::pair<std::string, std::string>("policy","http://www.nordugrid.org/ws/schemas/policy-arc"));

  res = node.XPathLookup("//policy:Policy", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    id = (std::string)(nd.Attribute("PolicyId"));

    //Setup the rules combining algorithm inside one policy
    if(nd.Attribute("CombiningAlg"))
      comalg = algfactory->createAlg((std::string)(nd.Attribute("CombiningAlg")));
    else comalg = algfactory->createAlg("Deny-Overrides");
    
    description = (std::string)(nd["Description"]);  
  }
  
  std::cout<<id<<std::endl;
  std::cout<<"Alg is:--------"<<comalg->getalgId()<<std::endl;
 
  for ( int i=0;; i++ ){
    rnd = nd["Rule"][i];
    if(!rnd) break;
    rule = new ArcRule(rnd);
    subelements.push_back(rule);
  }
    
}

MatchResult ArcPolicy::match(EvaluationCtx* ctx){
  //Arc::RequestTuple evaltuple = ctx->getEvalTuple();
  
  //Because ArcPolicy definition has no any <Subject, Resource, Action, Environment> directly;
  //All the <Subject, Resource, Action, Environment>s are only in ArcRule.
  //So the function always return "Match" 

  return MATCH;
  
}

Result ArcPolicy::eval(EvaluationCtx* ctx){
  
  Result result = comalg->combine(ctx, subelements);

  return result;
   
}

ArcPolicy::~ArcPolicy(){
  while(!subelements.empty()){
      delete subelements.back();
      subelements.pop_back();
  }

}
