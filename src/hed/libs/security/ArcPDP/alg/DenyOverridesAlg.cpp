#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DenyOverridesAlg.h"

namespace ArcSec{

std::string DenyOverridesCombiningAlg::algId = "Deny-Overrides";

Result DenyOverridesCombiningAlg::combine(EvaluationCtx* ctx, std::list<Policy*> policies){
  bool atleast_onepermit = false;
  bool atleast_oneerror = false;
  bool potentialdeny = false;
 
  std::list<Policy*>::iterator it;
  for(it = policies.begin(); it != policies.end(); it++) {
    Policy* policy = *it;
    Result res = policy->eval(ctx);

    //If get a return DECISION_DENY, then regardless of whatelse result from the other Rule,
    //always return DENY 
    if(res == DECISION_DENY)
      return DECISION_DENY;

    //If get a return DECISION_INDETERMINATE (this usually happens when there is something
    //wrong with the Attribute, like can not find Attribute with corrsponding AttributeId from RequestItem),
    //keep track of these cases.
    if(res == DECISION_INDETERMINATE) {
      atleast_oneerror = true;
 
      // If the Rule's effect is DENY, then this algorithm can return PERMIT,
      // since this Rule might have denied if it can do its stuff
      if((policy->getEffect()).compare("Deny")==0)
        potentialdeny = true;
    }
    //Keep track of whether we had at least one rule that is pertained to the request
    else if(res == DECISION_PERMIT)
      atleast_onepermit = true;
  } 
  
  if(potentialdeny) return DECISION_INDETERMINATE;

  //Some Rule said PERMIT, so since nothing could have denied, return PERMIT
  if(atleast_onepermit) return DECISION_PERMIT;

  //Nothing said PERMIT, but if there is problem with one of the Rules, then return INDETERMINATE
  if(atleast_oneerror) return DECISION_INDETERMINATE;

  //If here, none of the rules actually applied, return NOT_APPLICABLE
  return DECISION_NOT_APPLICABLE;
}

} //namespace ArcSec


