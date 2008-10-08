#ifndef __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__
#define __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__

#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {
///Implement the "Permit-Overrides" algorithm
/**Permit-Overrides, scans the policy set which is given as the parameters of "combine"
 *method, if gets "permit" result from any policy, then stops scanning and gives "permit"
 *as result, otherwise gives "deny".
 */
class PermitOverridesCombiningAlg : public CombiningAlg {
private:
  static std::string algId;

public:
  PermitOverridesCombiningAlg(){};
  virtual ~PermitOverridesCombiningAlg(){};

public:
 /**If there is one policy which return positive evaluation result, then omit the
 *other policies and return DECISION_PERMIT
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

#endif /* __ARC_SEC_PERMITOVERRIDESCOMBININGALG_H__ */

