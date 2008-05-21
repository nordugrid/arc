#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include <arc/loader/ClassLoader.h>

#include <fstream>
#include <iostream>
#include "PolicyParser.h"
#include "PolicyStore.h"

/*
//Should we provide different ClassLoader for different "get" function with different arguments?
static Arc::LoadableClass* get_policy_store (const std::list<std::string>& filelist, const std::string& alg) {
    return new ArcSec::PolicyStore(filelist, alg);
}

loader_descriptors __arc_policystore_modules__  = {
    { "policy.store", 0, &get_policy_store },
    { NULL, 0, NULL }
};
*/

using namespace Arc;
using namespace ArcSec;

PolicyStore::PolicyStore(const std::list<std::string>& filelist, const std::string& alg, EvaluatorContext* ctx){
  //combalg = alg;
  PolicyParser plparser;
  
  //call parsePolicy to parse each policies
  for(std::list<std::string>::const_iterator it = filelist.begin(); it != filelist.end(); it++){
    policies.push_back(PolicyElement(plparser.parsePolicy(*it, ctx)));
  }    
}

//Policy list  
//there also can be a class "PolicySet", which includes a few policies
std::list<PolicyStore::PolicyElement> PolicyStore::findPolicy(EvaluationCtx*) { //ctx){
  //For the existing Arc policy expression, we only need to return all the policies, because there is 
  //no Target definition in ArcPolicy (the Target is only in ArcRule)

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

void PolicyStore::addPolicy(const std::string& policyfile, EvaluatorContext* ctx, const std::string& id) {
  PolicyParser plparser;
  policies.push_back(PolicyElement(plparser.parsePolicy(policyfile, ctx),id));
}

void PolicyStore::removePolicies(void) {
  while(!(policies.empty())){
    delete (Policy*)(policies.back());
    policies.pop_back();
  }
}

PolicyStore::~PolicyStore(){
  removePolicies();
}
