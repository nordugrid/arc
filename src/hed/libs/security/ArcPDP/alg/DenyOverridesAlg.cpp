#include "DenyOverridesAlg.h"
#include "Result.h"

Result DenyOverridesAlg::combine(Evaluation* ctx, std::list<Policy*> policies){
  boolean atleast_onepermit = false;
  boolean atleast_oneerror = false;
  boolean potentialdeny = false;
 
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
        atlease_oneerror = true;
  
        if(policy->hasDirectEffect()&&policy->getEffect()==DECISION_DENY)
          potentialdeny = true; 
      }
    }
    else if(match == INDETERMINATE)
      atlease_oneerror = true;
  }
  
  if(potentialdeny) return DECISION_INDETERMINATE;
  if(atleast_onepermit) return DECISION_PERMIT;
  if(atleasr_oneerror) return DECISION_INDETERMINATE;
  return DECISION_NOT_APPLICABLE;
}



