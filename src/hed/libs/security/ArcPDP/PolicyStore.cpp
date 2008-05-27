#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include "PolicyParser.h"
#include "PolicyStore.h"

using namespace Arc;
using namespace ArcSec;

PolicyStore::PolicyStore(const std::list<std::string>& filelist, const std::string& alg, const std::string& policyclassname, EvaluatorContext* ctx){
  //combalg = alg;
  policy_classname = policyclassname;

  PolicyParser plparser;  
  //call parsePolicy to parse each policies
  for(std::list<std::string>::const_iterator it = filelist.begin(); it != filelist.end(); it++){
    policies.push_back(PolicyElement(plparser.parsePolicy(*it,policy_classname, ctx)));
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
  policies.push_back(PolicyElement(plparser.parsePolicy(policyfile, policy_classname, ctx),id));
}

void PolicyStore::addPolicy(BasePolicy* policy, EvaluatorContext* ctx,const std::string& id) {
  Policy* pls = dynamic_cast<Policy*>(policy);
  if(pls!=NULL) {
    policies.push_back(PolicyElement(pls, id));
  }
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
