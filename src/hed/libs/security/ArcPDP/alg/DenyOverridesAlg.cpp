#include "DenyOverridesAlg.h"

using namespace Arc;

Result DenyOverridesCombiningAlg::combine(EvaluationCtx* ctx, std::list<Policy*> policies){
  bool atleast_onepermit = false;
  bool atleast_oneerror = false;
  bool potentialdeny = false;
 
  std::list<Policy*>::iterator it;
  for(it = policies.begin(); it != policies.end(); it++) {
    Policy* policy = *it;
    MatchResult match = policy->match(ctx);
    
    //evaluate the policy, if one policy evaluation return negative result, then return DECISION_DENY
    if(match == MATCH) {
      Result res = policy->eval(ctx);
      if(res == DECISION_DENY)
        return DECISION_DENY;

      if(res == DECISION_PERMIT)
        atleast_onepermit = true;
      else if(res == DECISION_INDETERMINATE){
        //some indeterminate caused by "Condition"
        atleast_oneerror = true;
  
        if((policy->getEffect()).compare("Deny")==0)
          potentialdeny = true; 
      }
    }
    else if(match == INDETERMINATE)
      atleast_oneerror = true;
  }
  
  if(potentialdeny) return DECISION_INDETERMINATE;
  if(atleast_onepermit) return DECISION_PERMIT;
  if(atleast_oneerror) return DECISION_INDETERMINATE;
  return DECISION_NOT_APPLICABLE;
}



