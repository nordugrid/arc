#include "PolicyStore.h"
#include <fstream>
#include <iostream>
#include "ArcPolicy.h" 
#include "PolicyParser.h"
#include "../Result.h"

PolicyStore::PolicyStore(const std::list<std::string>& filelist, const std::string& alg){
  combalg = alg;
  for(std::list<std::string>::iterator it = filelist.begin(); it != filelist.end(); it++){
    policies.push_back(PolicyParser::parsePolicy(*it));
  }    

}

//use list, there also can be a class "PolicySet", which includes a few policies
std::list<Arc::Policy*> PolicyStore::findPolicy(const EvaluationCtx* ctx){
 
  std::list<Policy*>::iterator it; 
  for(it = policies.begin(); it!=policies.end(); it++ ){
    Match res = (*it)->match(ctx);
    if (res == MATCH )
  }

 //TODO 
}
