#ifndef __ARC_SEC_DENYOVERRIDESCOMBININGALG_H__
#define __ARC_SEC_DENYOVERRIDESCOMBININGALG_H__

#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {
///Implement the "Deny-Overrides" algorithm
/**Deny-Overrides, scans the policy set which is given as the parameters of "combine"
 *method, if gets "deny" result from any policy, then stops scanning and gives "deny"
 *as result, otherwise gives "permit". 
 */
class DenyOverridesCombiningAlg : public CombiningAlg {
private:
  static std::string algId;
public:
  DenyOverridesCombiningAlg(){};
  virtual ~DenyOverridesCombiningAlg(){};

public:
 /**If there is one policy which return negative evaluation result, then omit the 
 *other policies and return DECISION_DENY 
 *@param ctx  This object contains request information which will be used to evaluated
 *against policy.
 *@param policlies This is a container which contains policy objects.
 *@return The combined result according to the algorithm. 
 */
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies);

 /**Get the identifier*/ 
  virtual const std::string& getalgId(void) const {return algId;};
};

} // namespace ArcSec

#endif /* __ARC_SEC_DENYOVERRIDESCOMBININGALG_H__ */

