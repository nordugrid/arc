#include "DenyOverridesAlg.h"
#include "Result.h"

Result DenyOverridesAlg::combine(Evaluation* ctx, std::list<Policy*> policies){
  boolean atleast_onepermit = false;

  std::list<Policy*>::iterator it;
  for(it = policies.begin(); it != policies.end(); it++) {
    Policy* policy = *it;
    MatchResult match = policy->match(ctx);
    
    //evaluate the policy, if one policy evaluation return negative result, then return DECISION_DENY
    if(match == MATCH) {
      Result res = policy->eval(ctx);
      if(res == DECISION_DENY || res == DECISION_INDETERMINATE)
        return DECISION_DENY;
      if(res == DECISION_PERMIT)
        atleast_onepermit = true;
    }
  }
  
  if(atleast_onepermit) return DECISION_PERMIT;
  else return DECISION_NOT_APPLICABLE;
}



