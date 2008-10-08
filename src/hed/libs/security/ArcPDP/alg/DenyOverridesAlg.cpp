#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DenyOverridesAlg.h"

namespace ArcSec{

std::string DenyOverridesCombiningAlg::algId = "Deny-Overrides";

Result DenyOverridesCombiningAlg::combine(EvaluationCtx* ctx, std::list<Policy*> policies){
  bool atleast_onepermit = false;
  bool atleast_onenotapplicable = false;
 
  std::list<Policy*>::iterator it;
  for(it = policies.begin(); it != policies.end(); it++) {
    Policy* policy = *it;
    Result res = policy->eval(ctx);

    //If get a return DECISION_DENY, then regardless of whatelse result from the other Rule,
    //always return DENY 
    if(res == DECISION_DENY)
      return DECISION_DENY;

    //If get a return DECISION_NOT_APPLICABLE (this usually happens when Attribute with corrsponding
    //AttributeId can be found from RequestItem, but value does not match).
    if(res == DECISION_NOT_APPLICABLE) {
      atleast_onenotapplicable = true;
    }
    //Keep track of whether we had at least one rule that is pertained to the request
    else if(res == DECISION_PERMIT)
      atleast_onepermit = true;
  } 
  
  //Some Rule said PERMIT, so since nothing could have denied, return PERMIT
  if(atleast_onepermit) return DECISION_PERMIT;

  //No Rule said DENY, none of the rules actually applied, return NOT_APPLICABLE
  if(atleast_onenotapplicable) return DECISION_NOT_APPLICABLE;

  //If here, there is problem with one of the Rules, then return INDETERMINATE
  return DECISION_INDETERMINATE;
}

} //namespace ArcSec


