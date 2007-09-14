#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PolicyStore.h"
#include <fstream>
#include <iostream>
#include "Policy.h" 
#include "PolicyParser.h"

using namespace Arc;

PolicyStore::PolicyStore(const std::list<std::string>& filelist, const std::string& alg){
  combalg = alg;
  PolicyParser plparser;
  for(std::list<std::string>::const_iterator it = filelist.begin(); it != filelist.end(); it++){
    policies.push_back(plparser.parsePolicy(*it));
  }    

}

//use list, there also can be a class "PolicySet", which includes a few policies
std::list<Arc::Policy*> PolicyStore::findPolicy(EvaluationCtx* ctx){
  //For the existing Arc policy expression, we only need to return all the policies, because there is no Target definition in ArcPolicy (the Target is only in ArcRule)

  return policies;

/* 
  std::list<Policy*> ret;
  std::list<Policy*>::iterator it; 
  for(it = policies.begin(); it!=policies.end(); it++ ){
    MatchResult res = (*it)->match(ctx);
    if (res == MATCH )
      ret.push_back(*it);
  }
  return ret;
*/
 //TODO 
}
