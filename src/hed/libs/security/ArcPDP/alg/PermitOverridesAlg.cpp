#include "PermitOverridesAlg.h"
#include "Result.h"

Result PermitOverridesAlg::combine(Evaluation* ctx, std::list<Policy*> policies){
  boolean atleast_onedeny = false;
  boolean atleast_oneerror = false;
  boolean potentialpermit = false;

  std::list<Policy*>::iterator it;
  for(it = policies.begin(); it != policies.end(); it++) {
    Policy* policy = *it;
    MatchResult match = policy->match(ctx);
    
    //evaluate the policy, if one policy evaluation return positive result, then return DECISION_PERMIT
    if(match == MATCH) {
      Result res = policy->eval(ctx);
      if(res == DECISION_PERMIT)
        return DECISION_PERMIT;
      
      if (res == DECISION_INDETERMINATE){
        atleast_oneerror = true;
        if(policy->hasDirectEffect()&&policy->getEffect()==DECISION_PERMIT)
          potentialpermit = true;
      }
      else if(res == DECISION_DENY)
        atleast_onedeny = true;
    }
    else if(match == INDETERMINATE)
      atleast_oneerror = true;
  }
  
  if(potentialpermit) return DECISION_INDETERMINATE;
  if(atleast_onedeny) return DECISION_DENY;
  if(atleast_oneerror) return DECISION_INDETERMINATE;

  return DECISION_NOT_APPLICABLE;
}



