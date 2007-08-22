#include "ArcPolicy.h"
#include <list>

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

Result ArcPolicy::eval(const EvaluationCtx* ctx){

}

ArcPolicy::~ArcPolicy(){
  while(!subelements.empty()){
      delete subelements.back();
      subelements.pop_back();
  }

}
