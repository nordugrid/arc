#include "PolicyStore.h"
#include <fstream>
#include <iostream>
#include "ArcPolicy.h" 
#include "PolicyParser.h"

PolicyStore::PolicyStore(const std::list<std::string>& filelist, const std::string& alg){
  combalg = alg;
  for(std::list<std::string>::iterator it = filelist.begin(); it != filelist.end(); it++){
    policies.push_back(PolicyParser::parsePolicy(*it));
  }    

}

Arc::MatchedItem PolicyStore::findPolicy(const EvaluationCtx* ctx){
 
  std::list<Policy*>::iterator it; 
  for(it = policies.begin(); it!=policies.end(); it++ ){
    Arc::ReqItemList reqitems = (*it)->match(ctx);
  }

 //TODO 
}
