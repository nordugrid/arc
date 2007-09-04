#include "ArcPolicy.h"
#include "ArcRule.h"
#include <list>
#include "../alg/ArcAlgFactory.h"

using namespace Arc;

ArcPolicy::ArcPolicy(XMLNode& node) : Policy(node) {
  ArcRule *rule;
  XMLNode root, nd;
  ArcAlgFactory *algfactory = new ArcAlgFactory(); 

  std::string xml;
  node.GetXML(xml); //for testing
  std::cout << xml << std::endl;
  
  root = node.Child();
  id = (std::string)(root.Attribute("PolicyID"));

  //Setup the rules combining algorithm inside one policy
  if(root.Attribute("CombiningAlg"))
    comalg = algfactory->createAlg((std::string)(root.Attribute("CombiningAlg")));
  else comalg = algfactory->createAlg("Deny-Overrides");     

  description = (std::string)(root["Description"]);
  
  for ( int i=0;; i++ ){
    nd = root["Rule"][i];
    if(!nd) break;
    rule = new ArcRule(nd);

  node.GetXML(xml); //for testing
  std::cout << xml << std::endl;

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
