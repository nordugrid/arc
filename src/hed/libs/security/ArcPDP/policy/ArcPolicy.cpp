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
    rules.push_back(rule);
  }
    
}

Result ArcPolicy::eval(EvalCtx ctx){

}

ArcPolicy::~ArcPolicy(){
  while(!rules.empty()){
      delete rules.back();
      rules.pop_back();
  }

}
