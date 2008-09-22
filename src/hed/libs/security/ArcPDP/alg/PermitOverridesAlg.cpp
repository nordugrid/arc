#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PermitOverridesAlg.h"

namespace ArcSec{

std::string PermitOverridesCombiningAlg::algId = "Permit-Overrides";

Result PermitOverridesCombiningAlg::combine(EvaluationCtx* ctx, std::list<Policy*> policies){
  bool atleast_onedeny = false;
  bool atleast_oneerror = false;
  bool potentialpermit = false;

  std::list<Policy*>::iterator it;
  for(it = policies.begin(); it != policies.end(); it++) {
    Policy* policy = *it;
    Result res = policy->eval(ctx);

    //If get a return DECISION_PERMIT, then regardless of whatelse result from the other Rule,
    //always return PERMIT.
    if(res == DECISION_PERMIT)
      return DECISION_PERMIT;
    
    //If get a return DECISION_INDETERMINATE (this usually happens when there is something 
    //wrong with the Attribute, like can not find Attribute with corrsponding AttributeId from RequestItem), 
    //keep track of these cases. 
    if (res == DECISION_INDETERMINATE){
      atleast_oneerror = true;
 
      // If the Rule's effect is PERMIT, then this algorithm can not DENY, 
      // since this Rule might have permitted if it can do its stuff
      if((policy->getEffect()).compare("Permit")==0)
        potentialpermit = true;
    }
    //Keep track of whether we had at least one rule that is pertained to the request
    else if(res == DECISION_DENY)
      atleast_onedeny = true;
  }
 
  if(potentialpermit) return DECISION_INDETERMINATE;

  //Some Rule said DENY, so since nothing could have permitted, return DENY
  if(atleast_onedeny) return DECISION_DENY;

  //No Rule said DENY, but if there is problem with one of the Rules, then return INDETERMINATE
  if(atleast_oneerror) return DECISION_INDETERMINATE;

  //If here, none of the rules actually applied, return NOT_APPLICABLE
  return DECISION_NOT_APPLICABLE;
}

} //namespace ArcSec

