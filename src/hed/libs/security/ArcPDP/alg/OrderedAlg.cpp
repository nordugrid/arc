#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "OrderedAlg.h"

//   Evaluation results
// PERMIT
// DENY
// INDETERMINATE
// NOT_APPLICABLE

namespace ArcSec{

std::string PermitDenyIndeterminateNotApplicableCombiningAlg::algId = "Permit-Deny-Indeterminate-NotApplicable";
std::string PermitDenyNotApplicableIndeterminateCombiningAlg::algId = "Permit-Deny-NotApplicable-Indeterminate";
std::string PermitIndeterminateDenyNotApplicableCombiningAlg::algId = "Permit-Indeterminate-Deny-NotApplicable";
std::string PermitIndeterminateNotApplicableDenyCombiningAlg::algId = "Permit-Indeterminate-NotApplicable-Deny";
std::string PermitNotApplicableDenyIndeterminateCombiningAlg::algId = "Permit-NotApplicable-Deny-Indeterminate";
std::string PermitNotApplicableIndeterminateDenyCombiningAlg::algId = "Permit-NotApplicable-Indeterminate-Deny";
std::string DenyPermitIndeterminateNotApplicableCombiningAlg::algId = "Deny-Permit-Indeterminate-NotApplicable";
std::string DenyPermitNotApplicableIndeterminateCombiningAlg::algId = "Deny-Permit-NotApplicable-Indeterminate";
std::string DenyIndeterminatePermitNotApplicableCombiningAlg::algId = "Deny-Indeterminate-Permit-NotApplicable";
std::string DenyIndeterminateNotApplicablePermitCombiningAlg::algId = "Deny-Indeterminate-NotApplicable-Permit";
std::string DenyNotApplicablePermitIndeterminateCombiningAlg::algId = "Deny-NotApplicable-Permit-Indeterminate";
std::string DenyNotApplicableIndeterminatePermitCombiningAlg::algId = "Deny-NotApplicable-Indeterminate-Permit";
std::string IndeterminatePermitDenyNotApplicableCombiningAlg::algId = "Indeterminate-Permit-Deny-NotApplicable";
std::string IndeterminatePermitNotApplicableDenyCombiningAlg::algId = "Indeterminate-Permit-NotApplicable-Deny";
std::string IndeterminateDenyPermitNotApplicableCombiningAlg::algId = "Indeterminate-Deny-Permit-NotApplicable";
std::string IndeterminateDenyNotApplicablePermitCombiningAlg::algId = "Indeterminate-Deny-NotApplicable-Permit";
std::string IndeterminateNotApplicablePermitDenyCombiningAlg::algId = "Indeterminate-NotApplicable-Permit-Deny";
std::string IndeterminateNotApplicableDenyPermitCombiningAlg::algId = "Indeterminate-NotApplicable-Deny-Permit";
std::string NotApplicablePermitDenyIndeterminateCombiningAlg::algId = "NotApplicable-Permit-Deny-Indeterminate";
std::string NotApplicablePermitIndeterminateDenyCombiningAlg::algId = "NotApplicable-Permit-Indeterminate-Deny";
std::string NotApplicableDenyPermitIndeterminateCombiningAlg::algId = "NotApplicable-Deny-Permit-Indeterminate";
std::string NotApplicableDenyIndeterminatePermitCombiningAlg::algId = "NotApplicable-Deny-Indeterminate-Permit";
std::string NotApplicableIndeterminatePermitDenyCombiningAlg::algId = "NotApplicable-Indeterminate-Permit-Deny";
std::string NotApplicableIndeterminateDenyPermitCombiningAlg::algId = "NotApplicable-Indeterminate-Deny-Permit";

Result PermitDenyIndeterminateNotApplicableCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_PERMIT,
  DECISION_DENY,
  DECISION_INDETERMINATE,
  DECISION_NOT_APPLICABLE
};

Result PermitDenyNotApplicableIndeterminateCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_PERMIT,
  DECISION_DENY,
  DECISION_NOT_APPLICABLE,
  DECISION_INDETERMINATE
};

Result PermitIndeterminateDenyNotApplicableCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_PERMIT,
  DECISION_INDETERMINATE,
  DECISION_DENY,
  DECISION_NOT_APPLICABLE
};

Result PermitIndeterminateNotApplicableDenyCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_PERMIT,
  DECISION_INDETERMINATE,
  DECISION_NOT_APPLICABLE,
  DECISION_DENY
};

Result PermitNotApplicableDenyIndeterminateCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_PERMIT,
  DECISION_NOT_APPLICABLE,
  DECISION_DENY,
  DECISION_INDETERMINATE
};

Result PermitNotApplicableIndeterminateDenyCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_PERMIT,
  DECISION_NOT_APPLICABLE,
  DECISION_INDETERMINATE,
  DECISION_DENY
};

Result DenyPermitIndeterminateNotApplicableCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_DENY,
  DECISION_PERMIT,
  DECISION_INDETERMINATE,
  DECISION_NOT_APPLICABLE
};

Result DenyPermitNotApplicableIndeterminateCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_DENY,
  DECISION_PERMIT,
  DECISION_NOT_APPLICABLE,
  DECISION_INDETERMINATE
};

Result DenyIndeterminatePermitNotApplicableCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_DENY,
  DECISION_INDETERMINATE,
  DECISION_PERMIT,
  DECISION_NOT_APPLICABLE
};

Result DenyIndeterminateNotApplicablePermitCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_DENY,
  DECISION_INDETERMINATE,
  DECISION_NOT_APPLICABLE,
  DECISION_PERMIT
};

Result DenyNotApplicablePermitIndeterminateCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_DENY,
  DECISION_NOT_APPLICABLE,
  DECISION_PERMIT,
  DECISION_INDETERMINATE
};

Result DenyNotApplicableIndeterminatePermitCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_DENY,
  DECISION_NOT_APPLICABLE,
  DECISION_INDETERMINATE,
  DECISION_PERMIT
};

Result IndeterminatePermitDenyNotApplicableCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_INDETERMINATE,
  DECISION_PERMIT,
  DECISION_DENY,
  DECISION_NOT_APPLICABLE
};

Result IndeterminatePermitNotApplicableDenyCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_INDETERMINATE,
  DECISION_PERMIT,
  DECISION_NOT_APPLICABLE,
  DECISION_DENY
};

Result IndeterminateDenyPermitNotApplicableCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_INDETERMINATE,
  DECISION_DENY,
  DECISION_PERMIT,
  DECISION_NOT_APPLICABLE
};

Result IndeterminateDenyNotApplicablePermitCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_INDETERMINATE,
  DECISION_DENY,
  DECISION_NOT_APPLICABLE,
  DECISION_PERMIT
};

Result IndeterminateNotApplicablePermitDenyCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_INDETERMINATE,
  DECISION_NOT_APPLICABLE,
  DECISION_PERMIT,
  DECISION_DENY
};

Result IndeterminateNotApplicableDenyPermitCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_INDETERMINATE,
  DECISION_NOT_APPLICABLE,
  DECISION_DENY,
  DECISION_PERMIT
};

Result NotApplicablePermitDenyIndeterminateCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_NOT_APPLICABLE,
  DECISION_PERMIT,
  DECISION_DENY,
  DECISION_INDETERMINATE
};

Result NotApplicablePermitIndeterminateDenyCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_NOT_APPLICABLE,
  DECISION_PERMIT,
  DECISION_INDETERMINATE,
  DECISION_DENY
};

Result NotApplicableDenyPermitIndeterminateCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_NOT_APPLICABLE,
  DECISION_DENY,
  DECISION_PERMIT,
  DECISION_INDETERMINATE
};

Result NotApplicableDenyIndeterminatePermitCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_NOT_APPLICABLE,
  DECISION_DENY,
  DECISION_INDETERMINATE,
  DECISION_PERMIT
};

Result NotApplicableIndeterminatePermitDenyCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_NOT_APPLICABLE,
  DECISION_INDETERMINATE,
  DECISION_PERMIT,
  DECISION_DENY
};

Result NotApplicableIndeterminateDenyPermitCombiningAlg::priorities[MAX_OREDERED_PRIORITIES] = {
  DECISION_NOT_APPLICABLE,
  DECISION_INDETERMINATE,
  DECISION_DENY,
  DECISION_PERMIT
};



Result OrderedCombiningAlg::combine(EvaluationCtx* ctx, std::list<Policy*> policies,const Result priorities[MAX_OREDERED_PRIORITIES]){
  std::list<Policy*>::iterator it;
  int occurencies[MAX_OREDERED_PRIORITIES];
  memset(occurencies,0,sizeof(occurencies));
  for(it = policies.begin(); it != policies.end(); it++) {
    Policy* policy = *it;
    Result res = policy->eval(ctx);
    for(int n = 0;n<MAX_OREDERED_PRIORITIES;++n) {
      if(priorities[n] == res) {
        ++(occurencies[n]);
        break;
      };
    };
    if(occurencies[0]) break;
  };
  for(int n = 0;n<MAX_OREDERED_PRIORITIES;++n) {
    if(occurencies[n]) return priorities[n];
  };
  return priorities[MAX_OREDERED_PRIORITIES-1];
}

} //namespace ArcSec


